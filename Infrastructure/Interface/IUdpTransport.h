/***
 * @Author: dawn 1964173012@qq.com
 * @Date: 2026-05-27 14:47:32
 * @LastEditors: dawn 1964173012@qq.com
 * @LastEditTime: 2026-05-27 14:53:25
 * @FilePath: /DOD-WTC3/code/Interface/IUdpTransport.h
 * @Description:
 * @
 * @Copyright (c) 2026 by AmsAudio Li YuMing, All Rights Reserved.
 */
#pragma once

#include <QString>
#include <QByteArray>

class IUdpTransport
{
public:
    virtual ~IUdpTransport() = default;

    virtual bool bind(quint16 port) = 0;

    /**
     * @brief 指定发送时使用的本机网卡 IPv4 地址（空串表示不指定，交给系统路由决定）。
     *
     * 多网卡场景下必须指定：socket 绑在 0.0.0.0 时，出口网卡完全由内核路由表决定；
     * 两块网卡掩码重叠（例如都是 /16）或存在多条等价路由时，广播包会从另一块网卡发出，
     * 目标设备收不到请求，本机也就收不到任何回包——表现就是「选了网卡却收不到数据」。
     * 绑定到具体网卡地址后，源地址即被钉死在该网卡上，出口随之确定。
     */
    virtual void setLocalAddress(const QString& ipv4) = 0;

    virtual bool sendTo(const QString& ip, quint16 port, const QByteArray& data)                                    = 0;
    virtual bool sendTo(quint16 localBindPort, const QString& ip, quint16 port, const QByteArray& data)             = 0;
    virtual bool tryReceiveFrom(quint16 port, QByteArray& data, QString& senderIp, quint16& senderPort)             = 0;
    virtual bool receiveFrom(quint16 port, QByteArray& data, QString& senderIp, quint16& senderPort, int timeoutMs) = 0;
};