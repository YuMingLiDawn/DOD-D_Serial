/***
 * @Author: dawn 1964173012@qq.com
 * @Date: 2026-11-26 17:05:48
 * @LastEditors: dawn 1964173012@qq.com
 * @LastEditTime: 2026-11-26 17:07:21
 * @FilePath: /DOD-D_Serial/Service/NicSearch/NicSearch.cpp
 * @Description: NicSearch class implementation
 * @
 * @Copyright (c) 2026 by AmsAudio Li YuMing, All Rights Reserved.
 */

#include "NicSearch.h"

#include "Infrastructure/Interface/INicProvider.h"
#include "Log.h"
#include "NicProvider.h"

#include <QTimer>
#include <QVariantMap>

namespace
{
    // 快照里的单条记录：影响「网卡是否变化」的字段都算进去
    QString nicKey(const NicInfo& nic)
    {
        return QStringLiteral("%1|%2|%3|%4|%5").arg(nic.name, nic.ipv4, nic.netmask, nic.mac, nic.broadcast);
    }
} // namespace

NicSearch::NicSearch(QObject* parent)
    : QObject(parent)
    , m_provider(std::make_unique<NicProvider>())
    , m_timer(new QTimer(this))
{
    m_timer->setInterval(m_intervalMs);
    m_timer->setTimerType(Qt::CoarseTimer); // 2s 级别的轮询不需要精确定时器
    connect(m_timer, &QTimer::timeout, this, &NicSearch::scan);

    // scan(); // 先同步扫一次，QML 首屏就有数据
}

NicSearch::~NicSearch() = default;

NicSearch& NicSearch::instance()
{
    static NicSearch instance;
    return instance;
}

bool NicSearch::isWatching() const
{
    return m_timer != nullptr && m_timer->isActive();
}

void NicSearch::setIntervalMs(int intervalMs)
{
    if (intervalMs <= 0 || intervalMs == m_intervalMs) {
        return;
    }
    m_intervalMs = intervalMs;
    m_timer->setInterval(m_intervalMs);
    emit intervalMsChanged();
    LOG_INFO("[NicSearch] 轮询间隔改为 {} ms", m_intervalMs);
}

QVariantList NicSearch::listWiredNics() const
{
    QVariantList list;
    const QVector<NicInfo> current = m_provider->listWiredNics();
    for (const NicInfo& nic : current) {
        QVariantMap item;
        item.insert(QStringLiteral("id"), nic.id);
        item.insert(QStringLiteral("name"), nic.name);
        item.insert(QStringLiteral("ipv4"), nic.ipv4);
        item.insert(QStringLiteral("netmask"), nic.netmask);
        item.insert(QStringLiteral("mac"), nic.mac);
        item.insert(QStringLiteral("broadcast"), nic.broadcast);
        item.insert(QStringLiteral("gateway"), nic.gateway);
        list.append(item);
    }
    return list;
}

QVector<NicInfo> NicSearch::listWiredNicsRaw() const
{
    return m_provider->listWiredNics();
}

void NicSearch::start()
{
    if (m_timer->isActive()) {
        return;
    }
    m_timer->start();
    emit watchingChanged();
    LOG_INFO("[NicSearch] 开始定时检测网卡，间隔 {} ms", m_intervalMs);
}

void NicSearch::stop()
{
    if (!m_timer->isActive()) {
        return;
    }
    m_timer->stop();
    emit watchingChanged();
    LOG_INFO("[NicSearch] 停止检测网卡");
}

void NicSearch::refresh()
{
    scan();
}

void NicSearch::scan()
{
    const QVector<NicInfo> current = m_provider->listWiredNics();

    QStringList  snapshot;
    QVariantList list;
    snapshot.reserve(current.size());
    list.reserve(current.size());

    for (const NicInfo& nic : current) {
        snapshot << nicKey(nic);

        QVariantMap item;
        item.insert(QStringLiteral("id"), nic.id);
        item.insert(QStringLiteral("name"), nic.name);
        item.insert(QStringLiteral("ipv4"), nic.ipv4);
        item.insert(QStringLiteral("netmask"), nic.netmask);
        item.insert(QStringLiteral("mac"), nic.mac);
        item.insert(QStringLiteral("broadcast"), nic.broadcast);
        item.insert(QStringLiteral("gateway"), nic.gateway);
        list.append(item);
    }

    if (snapshot == m_snapshot) {
        return; // 没变化就不打扰 QML，避免无意义的界面重绘
    }

    // 有变化：把「谁进来、谁出去」记进日志，排查现场很省事
    for (const QString& key : snapshot) {
        if (!m_snapshot.contains(key)) {
            LOG_INFO("[NicSearch] 网卡出现/变化: {}", key);
        }
    }
    for (const QString& key : m_snapshot) {
        if (!snapshot.contains(key)) {
            LOG_INFO("[NicSearch] 网卡消失/变化: {}", key);
        }
    }

    m_snapshot = snapshot;
    m_nics     = list;
    emit nicsChanged();
}
