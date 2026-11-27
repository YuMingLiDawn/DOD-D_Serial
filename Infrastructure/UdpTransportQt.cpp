/***
 * @Author: dawn 1964173012@qq.com
 * @Date: 2026-05-27 14:54:26
 * @LastEditors: dawn 1964173012@qq.com
 * @LastEditTime: 2026-06-08 16:20:00
 * @FilePath: /DOD-WTC3/code/Infrastructure/UdpTransportQt.cpp
 * @Description:
 * @
 * @Copyright (c) 2026 by AmsAudio Li YuMing, All Rights Reserved.
 */

#include "UdpTransportQt.h"

#include "Log.h"

#include <QNetworkAddressEntry>
#include <QNetworkInterface>

namespace
{
    // docker/veth/br-* 这类虚拟网卡加入组播没有意义，还会刷一堆告警
    bool isVirtualInterface(const QNetworkInterface& iface)
    {
        static const QStringList kPrefixes{QStringLiteral("docker"),  QStringLiteral("br-"), QStringLiteral("veth"),  QStringLiteral("virbr"), QStringLiteral("vmnet"),
                                           QStringLiteral("vboxnet"), QStringLiteral("tun"), QStringLiteral("tap"),   QStringLiteral("lo"),    QStringLiteral("utun"),
                                           QStringLiteral("awdl"),    QStringLiteral("llw"), QStringLiteral("bridge")};

        const QString name = iface.name();
        for (const QString& prefix : kPrefixes) {
            if (name.startsWith(prefix, Qt::CaseInsensitive)) {
                return true;
            }
        }
        return false;
    }

    bool hasIpv4(const QNetworkInterface& iface)
    {
        for (const QNetworkAddressEntry& entry : iface.addressEntries()) {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                return true;
            }
        }
        return false;
    }
} // namespace
UdpTransportQt::UdpTransportQt()
{}

UdpTransportQt::~UdpTransportQt()
{
    for (auto& pair : m_sockets) {
        pair.second->close();
    }
    m_sockets.clear();
    resetSendSockets();
}

bool UdpTransportQt::bind(quint16 port)
{
    if (m_sockets.find(port) != m_sockets.end()) {
        return true;
    }

    auto socket = std::make_unique<QUdpSocket>();
    if (!socket->bind(QHostAddress::AnyIPv4, port, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        LOG_WARN("[Udp] bind 接收 socket 失败 port={}: {}", port, socket->errorString());
        return false;
    }

    m_sockets.emplace(port, std::move(socket));
    return true;
}

bool UdpTransportQt::bind(const QString& localIp, quint16 port)
{
    if (localIp.isEmpty()) {
        return bind(port); // 空串 = 绑定 0.0.0.0
    }
    if (m_sockets.find(port) != m_sockets.end()) {
        return true;
    }

    const QHostAddress address(localIp);
    if (address.isNull() || address.protocol() != QAbstractSocket::IPv4Protocol) {
        LOG_WARN("[Udp] 绑定 IP 无效: {}", localIp);
        return false;
    }

    auto socket = std::make_unique<QUdpSocket>();
    if (!socket->bind(address, port, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        LOG_WARN("[Udp] bind 接收 socket 失败 {0}:{1}: {2}", localIp, port, socket->errorString());
        return false;
    }

    LOG_INFO("[Udp] 接收 socket 已绑定 {0}:{1}", localIp, port);
    m_sockets.emplace(port, std::move(socket));
    return true;
}

void UdpTransportQt::setLocalAddress(const QString& ipv4)
{
    if (ipv4 == m_localAddress) {
        return;
    }

    m_localAddress = ipv4;
    // 网卡变了：旧发送 socket 的源地址已经不对，全部丢弃，下次发送时按新地址重建
    resetSendSockets();
    LOG_INFO("[Udp] 发送出口网卡切换为 {}", ipv4.isEmpty() ? QStringLiteral("<any>") : ipv4);
}

void UdpTransportQt::resetSendSockets()
{
    for (auto& pair : m_sendSockets) {
        pair.second->close();
    }
    m_sendSockets.clear();
    m_sendSocketFailed.clear();
}

QUdpSocket* UdpTransportQt::ensureSendSocket(quint16 localBindPort)
{
    auto it = m_sendSockets.find(localBindPort);
    if (it != m_sendSockets.end()) {
        return it->second.get();
    }

    if (m_localAddress.isEmpty() || m_sendSocketFailed.find(localBindPort) != m_sendSocketFailed.end()) {
        return nullptr;
    }

    const QHostAddress localAddress(m_localAddress);
    if (localAddress.isNull() || localAddress.protocol() != QAbstractSocket::IPv4Protocol) {
        m_sendSocketFailed.insert(localBindPort);
        return nullptr;
    }

    auto socket = std::make_unique<QUdpSocket>();
    // 关键：源地址绑到选中网卡，端口与接收 socket 相同（设备按源 IP:端口回包）
    if (!socket->bind(localAddress, localBindPort, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        LOG_WARN("[Udp] bind 发送 socket 失败 {0}:{1}（{2}），回退到 0.0.0.0 socket", m_localAddress, localBindPort, socket->errorString());
        m_sendSocketFailed.insert(localBindPort);
        return nullptr;
    }

    LOG_INFO("[Udp] 发送 socket 已绑定网卡 {0}:{1}", m_localAddress, localBindPort);
    QUdpSocket* raw = socket.get();
    m_sendSockets.emplace(localBindPort, std::move(socket));
    return raw;
}

bool UdpTransportQt::writeDatagram(quint16 localBindPort, const QString& ip, quint16 port, const QByteArray& data)
{
    auto it = m_sockets.find(localBindPort);
    if (it == m_sockets.end()) {
        LOG_WARN("[Udp] 没有可用 socket 发送数据（端口 {} 未 bind）", localBindPort);
        return false;
    }

    // 优先使用绑定到选中网卡的发送 socket；未选网卡或绑定失败时退回 0.0.0.0 socket（由路由决定出口）
    QUdpSocket* socket = ensureSendSocket(localBindPort);
    if (!socket) {
        socket = it->second.get();
    }

    const QHostAddress address(ip);
    if (address.isNull()) {
        LOG_WARN("[Udp] 目标地址无效 ip={} port={}", ip, port);
        return false;
    }

    const qint64 bytesSent = socket->writeDatagram(data, address, port);
    if (bytesSent != data.size()) {
        LOG_WARN("[Udp] 发送失败 to {0}:{1}，error={2} localAddress={3}", ip, port, socket->errorString(), m_localAddress);
        return false;
    }
    return true;
}

bool UdpTransportQt::joinMulticastGroup(const QString& group, quint16 port, const QString& ifaceName)
{
    auto it = m_sockets.find(port);
    if (it == m_sockets.end()) {
        LOG_WARN("[Udp] 加入组播前必须先 bind 端口 {}（组播 {}）", port, group);
        return false;
    }

    const QHostAddress groupAddress(group);
    if (groupAddress.isNull()) {
        LOG_WARN("[Udp] 组播地址无效: {}", group);
        return false;
    }

    QUdpSocket* socket = it->second.get();

    QList<QNetworkInterface> ifaces;
    if (!ifaceName.isEmpty()) {
        const QNetworkInterface iface = QNetworkInterface::interfaceFromName(ifaceName);
        if (!iface.isValid()) {
            LOG_WARN("[Udp] 找不到网卡: {}", ifaceName);
            return false;
        }
        ifaces << iface;
    }
    else {
        for (const QNetworkInterface& iface : QNetworkInterface::allInterfaces()) {
            const auto flags = iface.flags();
            if (!flags.testFlag(QNetworkInterface::IsUp) || flags.testFlag(QNetworkInterface::IsLoopBack)) {
                continue;
            }
            if (isVirtualInterface(iface) || !hasIpv4(iface)) {
                continue; // docker/veth/无 IPv4 的网卡不参与组播
            }
            ifaces << iface;
        }
    }

    bool joined = false;
    for (const QNetworkInterface& iface : ifaces) {
        if (socket->joinMulticastGroup(groupAddress, iface)) {
            joined = true;
            LOG_INFO("[Udp] 已在网卡 {}（{}）上加入组播 {}", iface.humanReadableName(), iface.name(), group);
        }
        else {
            LOG_WARN("[Udp] 网卡 {} 加入组播 {} 失败: {}", iface.name(), group, socket->errorString());
        }
    }

    if (!joined && socket->joinMulticastGroup(groupAddress)) {
        joined = true;
        LOG_WARN("[Udp] 各网卡均未加入成功，已用默认网卡加入组播 {}", group);
    }

    // 本机发送的组播也能收到，方便自测
    if (joined) {
        socket->setSocketOption(QAbstractSocket::MulticastLoopbackOption, 1);
    }
    return joined;
}

bool UdpTransportQt::sendTo(const QString& ip, quint16 port, const QByteArray& data)
{
    if (m_sockets.empty()) {
        LOG_WARN("[Udp] 没有可用 socket 发送数据（尚未 bind 任何端口）");
        return false;
    }

    return writeDatagram(m_sockets.begin()->first, ip, port, data);
}

bool UdpTransportQt::sendTo(quint16 localBindPort, const QString& ip, quint16 port, const QByteArray& data)
{
    if (m_sockets.empty()) {
        return false;
    }

    const quint16 bindPort = (m_sockets.find(localBindPort) != m_sockets.end()) ? localBindPort : m_sockets.begin()->first;
    return writeDatagram(bindPort, ip, port, data);
}

bool UdpTransportQt::tryReceiveFrom(quint16 port, QByteArray& data, QString& senderIp, quint16& senderPort)
{
    auto it = m_sockets.find(port);
    if (it == m_sockets.end()) {
        return false;
    }
    QUdpSocket* socket = it->second.get();

    if (!socket->hasPendingDatagrams()) {
        return false;
    }

    qint64 pending = socket->pendingDatagramSize();
    if (pending <= 0)
        pending = 65536;

    data.resize(static_cast<int>(pending));
    QHostAddress host;
    quint16      portFrom = 0;
    qint64       read     = socket->readDatagram(data.data(), data.size(), &host, &portFrom);
    if (read < 0) {
        return false;
    }

    data.resize(static_cast<int>(read));
    senderIp   = host.toString();
    senderPort = portFrom;
    return true;
}

bool UdpTransportQt::receiveFrom(quint16 port, QByteArray& data, QString& senderIp, quint16& senderPort, int timeoutMs)
{
    auto it = m_sockets.find(port);
    if (it == m_sockets.end()) {
        return false;
    }
    QUdpSocket* socket = it->second.get();

    if (!socket->waitForReadyRead(timeoutMs)) {
        return false;
    }

    return tryReceiveFrom(port, data, senderIp, senderPort);
}

void UdpTransportQt::drainAllFrom(quint16 port, std::function<void(const QByteArray&, const QString&, quint16)> callback)
{
    auto it = m_sockets.find(port);
    if (it != m_sockets.end()) {
        drainSocket(it->second.get(), callback);
    }

    // 选定网卡后，设备回包是发往「网卡IP:port」的，内核会投递给绑定了该地址的发送 socket，
    // 这里必须一并排空，否则回包堆在这个 socket 里收不到。
    auto sendIt = m_sendSockets.find(port);
    if (sendIt != m_sendSockets.end()) {
        drainSocket(sendIt->second.get(), callback);
    }
}

void UdpTransportQt::drainSocket(QUdpSocket* socket, const std::function<void(const QByteArray&, const QString&, quint16)>& callback)
{
    if (!socket) {
        return;
    }

    while (socket->hasPendingDatagrams()) {
        qint64 pending = socket->pendingDatagramSize();
        if (pending <= 0)
            pending = 65536;

        QByteArray data;
        data.resize(static_cast<int>(pending));
        QHostAddress host;
        quint16      portFrom = 0;
        qint64       read     = socket->readDatagram(data.data(), data.size(), &host, &portFrom);
        if (read < 0) {
            break;
        }
        data.resize(static_cast<int>(read));

        QString senderIp   = host.toString();
        quint16 senderPort = portFrom;

        callback(data, senderIp, senderPort);
    }
}
