/***
 * @Author: dawn 1964173012@qq.com
 * @Date: 2026-05-26 18:22:29
 * @LastEditors: dawn 1964173012@qq.com
 * @LastEditTime: 2026-11-26 17:30:00
 * @FilePath: /DOD-D_Serial/Infrastructure/NicProvider.cpp
 * @Description: 网卡枚举（不读取网关，保持跨平台简单）
 * @
 * @Copyright (c) 2026 by AmsAudio Li YuMing, All Rights Reserved.
 */

#include "NicProvider.h"

#include "Log.h"

#include <QAbstractSocket>
#include <QHostAddress>
#include <QNetworkInterface>

NicProvider::NicProvider()
{}

QVector<NicInfo> NicProvider::listWiredNics() const
{
    QVector<NicInfo> nics;
    const auto       interfaces = QNetworkInterface::allInterfaces();

    for (const auto& iface : interfaces) {
        if (!isValidIntercface(iface)) {
            continue;
        }

        NicInfo nic;
        nic.id   = QString::number(iface.index());
        nic.name = iface.name();
        nic.mac  = iface.hardwareAddress();

        // 不再解析各平台路由表取网关：留空，需要时由上层自行设置/探测
        nic.gateway = QString();

        for (const auto& entry : iface.addressEntries()) {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                nic.ipv4      = entry.ip().toString();
                nic.netmask   = entry.netmask().toString();
                nic.broadcast = QStringLiteral("255.255.255.255");
                break;
            }
        }

        LOG_DEBUG("发现网卡: {} ({}/{})", nic.name, nic.ipv4, nic.netmask);
        nics.append(nic);
    }

    return nics;
}

bool NicProvider::hasIpv4Address(const QNetworkInterface& iface) const
{
    for (const auto& entry : iface.addressEntries()) {
        const auto& ip = entry.ip();

        if (ip.protocol() == QAbstractSocket::IPv4Protocol) {
            return true;
        }
    }

    return false;
}

bool NicProvider::hasBroadcastAddress(const QNetworkInterface& iface) const
{
    for (const auto& entry : iface.addressEntries()) {
        if (entry.ip().protocol() != QAbstractSocket::IPv4Protocol)
            continue;

        if (!entry.broadcast().isNull())
            return true;
    }

    return false;
}

bool NicProvider::isValidIntercface(const QNetworkInterface& iface) const
{
    if (!iface.flags().testFlag(QNetworkInterface::IsUp))
        return false;

    if (!iface.flags().testFlag(QNetworkInterface::IsRunning))
        return false;

    if (iface.flags().testFlag(QNetworkInterface::IsLoopBack))
        return false;

    if (isVirtualInterface(iface.name()))
        return false;

    if (!hasIpv4Address(iface))
        return false;

    if (!hasBroadcastAddress(iface))
        return false;

    return true;
}

bool NicProvider::isVirtualInterface(const QString& name) const
{
    static const QStringList prefixes{// Linux
                                      "docker", "br-", "veth",

                                      // VMware
                                      "vmnet",

                                      // VirtualBox
                                      "vboxnet",

                                      // macOS
                                      "utun", "awdl", "llw", "bridge"};

    for (const auto& prefix : prefixes) {
        if (name.startsWith(prefix, Qt::CaseInsensitive))
            return true;
    }

    return false;
}
