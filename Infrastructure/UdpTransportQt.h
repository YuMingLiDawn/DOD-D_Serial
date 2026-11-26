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
#include <QUdpSocket>
#include <QHostAddress>
#include <QByteArray>
#include <QString>
#include <map>
#include <set>
#include <memory>

class UdpTransportQt : public IUdpTransport
{
public:
    UdpTransportQt();
    ~UdpTransportQt() override;

    bool bind(quint16 port) override;
    void setLocalAddress(const QString& ipv4) override;
    bool sendTo(const QString& ip, quint16 port, const QByteArray& data) override;
    bool sendTo(quint16 localBindPort, const QString& ip, quint16 port, const QByteArray& data) override;
    bool tryReceiveFrom(quint16 port, QByteArray& data, QString& senderIp, quint16& senderPort) override;
    bool receiveFrom(quint16 port, QByteArray& data, QString& senderIp, quint16& senderPort, int timeoutMs) override;
    void drainAllFrom(quint16 port, std::function<void(const QByteArray&, const QString&, quint16)> callback);

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
