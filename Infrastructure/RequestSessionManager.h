/***
 * @Author: dawn 1964173012@qq.com
 * @Date: 2026-06-02 17:09:46
 * @LastEditors: dawn 1964173012@qq.com
 * @LastEditTime: 2026-06-03 20:52:24
 * @FilePath: /DOD-WTC3/code/Infrastructure/RequestSessionManager.h
 * @Description:
 * @
 * @Copyright (c) 2026 by AmsAudio Li YuMing, All Rights Reserved.
 */

#pragma once

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <QMap>
#include <QUuid>
#include <functional>

class IUdpTransport;

using ResponseMatcher = std::function<bool(const QByteArray& resp, const QString& senderIp, quint16 senderPort)>;

class RequestSessionManager : public QObject
{
    Q_OBJECT

public:
    explicit RequestSessionManager(IUdpTransport* transport, QObject* parent = nullptr);
    ~RequestSessionManager() override;

    // sendFunc should perform the send and return true on success
    QString submitRequest(const std::function<bool()>& sendFunc, int timeoutMs, int maxRetries, const ResponseMatcher& matcher);

    // convenience method for one-off matchers without retries
    QString submitMatcher(const ResponseMatcher& matcher, int timeoutMs);

    // called by transport owner when a raw response is received
    void onIncomingResponse(const QByteArray& resp, const QString& senderIp, quint16 senderPort);

    // called when a response does not match any pending request (e.g. device-initiated report)
    void setUnmatchHandler(std::function<void(const QByteArray&, const QString&, quint16)> handler);

    // sends a request without waiting for a matching response; caller owns timeout/retry logic
    void submitSendOnly(const std::function<bool()>& sendFunc);

    // Cancel every pending request. Each one is reported via
    // requestCompleted(id, false) so the caller can react. Does NOT
    // fire requestCompleted for requests that have no active timer
    // (already-completed, never-started).
    void cancelAll();

Q_SIGNALS:
    void requestCompleted(const QString& requestId, bool ok);

private Q_SLOTS:
    void onTimerTimeout();

private:
    struct Pending {
        QString               id;
        std::function<bool()> sendFunc;
        int                   timeoutMs;
        int                   retriesLeft;
        ResponseMatcher       matcher;
        QTimer*               timer = nullptr;
    };

    QMap<QString, Pending*>                                         m_pending;
    IUdpTransport*                                                  m_transport = nullptr; // not used directly but kept for future
    std::function<void(const QByteArray&, const QString&, quint16)> m_unmatchHandler;
    std::function<bool(const QByteArray&, const QString&, quint16)> m_matchHandler;
};
