/***
 * @Author: dawn 1964173012@qq.com
 * @Date: 2026-05-26 18:21:40
 * @LastEditors: dawn 1964173012@qq.com
 * @LastEditTime: 2026-05-26 18:23:16
 * @FilePath: /DOD-WTC3/code/Service/RadioManager/NicProvider.h
 * @Description:
 * @
 * @Copyright (c) 2026 by AmsAudio Li YuMing, All Rights Reserved.
 */

#pragma once

#include "Infrastructure/Interface/INicProvider.h"
#include <QNetworkInterface>

class NicProvider : public INicProvider
{
public:
    explicit NicProvider();
    QVector<NicInfo> listWiredNics() const override;

private:
    bool hasIpv4Address(const QNetworkInterface& iface) const;
    bool hasBroadcastAddress(const QNetworkInterface& iface) const;
    bool isValidIntercface(const QNetworkInterface& iface) const;
    bool isVirtualInterface(const QString& name) const;
};
