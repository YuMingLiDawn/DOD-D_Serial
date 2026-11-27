/***
 * @Author: dawn 1964173012@qq.com
 * @Date: 2026-05-27 14:54:29
 * @LastEditors: dawn 1964173012@qq.com
 * @LastEditTime: 2026-06-08 16:20:00
 * @FilePath: /DOD-WTC3/code/Infrastructure/UdpTransportQt.h
 * @Description:
 * @
 * @Copyright (c) 2026 by AmsAudio Li YuMing, All Rights Reserved.
 */

#pragma once

#include "Infrastructure/Interface/IUdpTransport.h"
#include <QByteArray>
#include <QHostAddress>
#include <QString>
#include <QUdpSocket>
#include <functional>
#include <map>
#include <memory>
#include <set>

class UdpTransportQt : public IUdpTransport
{
public:
    UdpTransportQt();
    ~UdpTransportQt() override;

    bool bind(quint16 port) override;

    // 绑定到指定本机 IPv4（空串 = 0.0.0.0）。多网卡/组播时用它钉死收包网卡。
    bool bind(const QString& localIp, quint16 port);
    void setLocalAddress(const QString& ipv4) override;
    bool sendTo(const QString& ip, quint16 port, const QByteArray& data) override;
    bool sendTo(quint16 localBindPort, const QString& ip, quint16 port, const QByteArray& data) override;
    bool tryReceiveFrom(quint16 port, QByteArray& data, QString& senderIp, quint16& senderPort) override;
    bool receiveFrom(quint16 port, QByteArray& data, QString& senderIp, quint16& senderPort, int timeoutMs) override;
    void drainAllFrom(quint16 port, std::function<void(const QByteArray&, const QString&, quint16)> callback);

    // 加入组播组（例如 239.255.254.253），必须在对应 port 上 bind() 之后调。
    // ifaceName 为空：在所有 up 且非回环网卡上加入；指定时只在该网卡上加入。
    // 返回是否至少成功加入一块网卡。
    bool joinMulticastGroup(const QString& group, quint16 port, const QString& ifaceName = QString());

private:
    // 绑定到选中网卡 IP + 同一本地端口的发送 socket；未选网卡或绑定失败时返回 nullptr
    QUdpSocket* ensureSendSocket(quint16 localBindPort);
    bool        writeDatagram(quint16 localBindPort, const QString& ip, quint16 port, const QByteArray& data);
    void        drainSocket(QUdpSocket* socket, const std::function<void(const QByteArray&, const QString&, quint16)>& callback);
    void        resetSendSockets();

private:
    std::map<quint16, std::unique_ptr<QUdpSocket>> m_sockets;
    std::map<quint16, std::unique_ptr<QUdpSocket>> m_sendSockets;
    std::set<quint16>                              m_sendSocketFailed;
    QString                                        m_localAddress;
};
