/***
 * @Author: dawn 1964173012@qq.com
 * @Date: 2026-06-02 17:09:46
 * @LastEditors: dawn 1964173012@qq.com
 * @LastEditTime: 2026-06-03 20:52:19
 * @FilePath: /DOD-WTC3/code/Infrastructure/RequestSessionManager.cpp
 * @Description:
 * @
 * @Copyright (c) 2026 by AmsAudio Li YuMing, All Rights Reserved.
 */

#include "Infrastructure/RequestSessionManager.h"
#include "Infrastructure/Interface/IUdpTransport.h"
#include "Service/Logger/GoogleLogger.h"
#include <QStringList>
#include <QVariantMap>

RequestSessionManager::RequestSessionManager(IUdpTransport* transport, QObject* parent)
    : QObject(parent)
    , m_transport(transport)
{}

RequestSessionManager::~RequestSessionManager()
{
    for (auto it = m_pending.begin(); it != m_pending.end(); ++it) {
        delete it.value()->timer;
        delete it.value();
    }
    m_pending.clear();
}

QString RequestSessionManager::submitRequest(const std::function<bool()>& sendFunc, int timeoutMs, int maxRetries, const ResponseMatcher& matcher)
{
    QString  id    = QUuid::createUuid().toString();
    Pending* p     = new Pending();
    p->id          = id;
    p->sendFunc    = sendFunc;
    p->timeoutMs   = timeoutMs;
    p->retriesLeft = maxRetries;
    p->matcher     = matcher;

    // timer for this pending
    p->timer = new QTimer(this);
    p->timer->setSingleShot(true);
    connect(p->timer, &QTimer::timeout, this, &RequestSessionManager::onTimerTimeout);

    m_pending.insert(id, p);

    // perform initial send
    // 这里只关心"异常不要打断流程"：发送结果由超时/重试机制兜底，所以 ok 只写不读
    [[maybe_unused]] bool ok = false;
    try {
        ok = p->sendFunc();
    } catch (...) {
        ok = false;
    }

    // start timer regardless; if send failed we still wait and let retries handle
    p->timer->start(p->timeoutMs);
    return id;
}

QString RequestSessionManager::submitMatcher(const ResponseMatcher& matcher, int timeoutMs)
{
    QString id = QUuid::createUuid().toString();

    Pending* p = new Pending();

    p->id          = id;
    p->timeoutMs   = timeoutMs;
    p->retriesLeft = 0;
    p->matcher     = matcher;

    p->sendFunc = []() {
        return true;
    };

    p->timer = new QTimer(this);
    p->timer->setSingleShot(true);

    connect(p->timer, &QTimer::timeout, this, &RequestSessionManager::onTimerTimeout);

    p->timer->start(timeoutMs);

    m_pending.insert(id, p);

    return id;
}

void RequestSessionManager::onIncomingResponse(const QByteArray& resp, const QString& senderIp, quint16 senderPort)
{
    if (m_matchHandler) {
        bool matched = false;
        try {
            matched = m_matchHandler(resp, senderIp, senderPort);
        } catch (...) {
            matched = false;
        }

        if (matched) {
            // global matcher accepted -> complete all pending requests as ok
            QList<QString> ids;
            for (auto it = m_pending.begin(); it != m_pending.end(); ++it) {
                ids.append(it.key());
            }

            for (const QString& id : ids) {
                Pending* p = m_pending.take(id);
                if (p->timer) {
                    p->timer->stop();
                    delete p->timer;
                }
                emit requestCompleted(id, true);
                DLOG(INFO) << "Global matcher handled response. Completing request " << id.toStdString() << " as success.";
                delete p;
            }
            return;
        }
    }

    // iterate pending and try to match (snapshot first to avoid mutation during emit)
    QList<Pending*> pendingSnapshot;
    QList<QString>  matchedIds;
    for (auto it = m_pending.begin(); it != m_pending.end(); ++it) {
        pendingSnapshot.append(it.value());
    }

    for (Pending* p : pendingSnapshot) {
        // skip entries that were removed during emit callbacks
        if (!m_pending.contains(p->id)) {
            continue;
        }

        bool ok = false;
        try {
            ok = p->matcher(resp, senderIp, senderPort);
        } catch (...) {
            ok = false;
        }

        if (ok) {
            // matched
            emit requestCompleted(p->id, true);
#if 0
            DLOG_EVERY_N(INFO, 100) << "Request " << p->id.toStdString() << " matched and completed successfully.";
#endif
            p->timer->stop();
            delete p->timer;
            matchedIds.append(p->id);
        }
    }

    // cleanup matched
    for (const QString& id : matchedIds) {
        Pending* p = m_pending.take(id);
        delete p;
    }

    // if no pending matched, dispatch to unmatched handler (device-initiated reports)
    if (matchedIds.isEmpty() && m_unmatchHandler) {
        try {
            m_unmatchHandler(resp, senderIp, senderPort);
        } catch (...) {
            // ignore
        }
    }
}

void RequestSessionManager::setUnmatchHandler(std::function<void(const QByteArray&, const QString&, quint16)> handler)
{
    m_unmatchHandler = std::move(handler);
}

void RequestSessionManager::submitSendOnly(const std::function<bool()>& sendFunc)
{
    try {
        sendFunc();
    } catch (...) {
        // ignore send failure
    }
}

void RequestSessionManager::onTimerTimeout()
{
    QTimer* t = qobject_cast<QTimer*>(sender());
    if (!t)
        return;

    QString  foundId;
    Pending* found = nullptr;
    for (auto it = m_pending.begin(); it != m_pending.end(); ++it) {
        if (it.value()->timer == t) {
            foundId = it.key();
            found   = it.value();
            break;
        }
    }

    if (!found)
        return;

    if (found->retriesLeft > 0) {
        // retry
        found->retriesLeft--;
        // 同首次发送：结果由超时/重试兜底，sent 只用于吞掉异常
        [[maybe_unused]] bool sent = false;
        try {
            sent = found->sendFunc();
        } catch (...) {
            sent = false;
        }

        // restart timer
        found->timer->start(found->timeoutMs);
        return;
    }

    // no retries left -> fail
    // Remove from map BEFORE emitting signal to prevent re-entrant modification during iteration
    QString timedOutId    = found->id;
    QTimer* timedOutTimer = found->timer;
    m_pending.remove(foundId);

    emit requestCompleted(timedOutId, false);

    // cleanup after signal (no re-entrancy risk now)
    if (timedOutTimer) {
        timedOutTimer->stop();
        delete timedOutTimer;
    }
    delete found;
}

void RequestSessionManager::cancelAll()
{
    // Snapshot the keys; we mutate m_pending while emitting.
    const QStringList ids = m_pending.keys();
    for (const QString& id : ids) {
        Pending* p = m_pending.take(id);
        if (!p) {
            continue;
        }
        if (p->timer) {
            p->timer->stop();
            delete p->timer;
        }
        emit requestCompleted(id, false);
        delete p;
    }
}
