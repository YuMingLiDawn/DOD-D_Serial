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
#include "Service/Logger/GoogleLogger.h"
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
        LOG(WARNING) << "[Udp] bind receive socket on port " << port << " failed: " << socket->errorString().toStdString();
        return false;
    }

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
    LOG(INFO) << "[Udp] egress NIC switched to " << (ipv4.isEmpty() ? std::string("<any>") : ipv4.toStdString());
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
        LOG(WARNING) << "[Udp] bind send socket " << m_localAddress.toStdString() << ":" << localBindPort << " failed: " << socket->errorString().toStdString() << ", fallback to 0.0.0.0 socket";
        m_sendSocketFailed.insert(localBindPort);
        return nullptr;
    }

    LOG(INFO) << "[Udp] send socket bound to NIC " << m_localAddress.toStdString() << ":" << localBindPort;
    QUdpSocket* raw = socket.get();
    m_sendSockets.emplace(localBindPort, std::move(socket));
    return raw;
}

bool UdpTransportQt::writeDatagram(quint16 localBindPort, const QString& ip, quint16 port, const QByteArray& data)
{
    auto it = m_sockets.find(localBindPort);
    if (it == m_sockets.end()) {
        DLOG(WARNING) << "No socket available to send data.";
        return false;
    }

    // 优先使用绑定到选中网卡的发送 socket；未选网卡或绑定失败时退回 0.0.0.0 socket（由路由决定出口）
    QUdpSocket* socket = ensureSendSocket(localBindPort);
    if (!socket) {
        socket = it->second.get();
    }

    const QHostAddress address(ip);
    if (address.isNull()) {
        LOG(WARNING) << "[Udp] invalid destination ip=" << ip.toStdString() << " port=" << port;
        return false;
    }

    const qint64 bytesSent = socket->writeDatagram(data, address, port);
    if (bytesSent != data.size()) {
        LOG(WARNING) << "[Udp] send to " << ip.toStdString() << ":" << port << " failed, error=" << socket->errorString().toStdString() << " localAddress=" << m_localAddress.toStdString();
        return false;
    }
    return true;
}

bool UdpTransportQt::sendTo(const QString& ip, quint16 port, const QByteArray& data)
{
    if (m_sockets.empty()) {
        DLOG(WARNING) << "No socket available to send data.";
        return false;
    }

    return writeDatagram(m_sockets.begin()->first, ip, port, data);
}

bool UdpTransportQt::sendTo(quint16 localBindPort, const QString& ip, quint16 port, const QByteArray& data)
{
    if (m_sockets.empty()) {
        DLOG(WARNING) << "No socket available to send data.";
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
    DLOG(INFO) << "<<<<<< UDP RX " << senderIp.toStdString() << ":" << senderPort << " len=" << read << " hex=" << data.toHex(' ').toStdString()
               << " str=" << QString(data).replace('\n', "\\n").replace('\r', "\\r").toStdString();
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
#if defined(DEBUG)
        DLOG(INFO) << "<<<<<< UDP 收到数据 " << senderIp.toStdString() << ":" << senderPort << " len=" << read << " hex=" << data.toHex(' ').toStdString()
                   << " str=" << QString(data).replace('\n', "\\n").replace('\r', "\\r").toStdString();
#endif
        callback(data, senderIp, senderPort);
    }
}
