/***
 * @Author: dawn 1964173012@qq.com
 * @Date: 2026-05-26 18:10:18
 * @LastEditors: dawn 1964173012@qq.com
 * @LastEditTime: 2026-05-26 18:10:23
 * @FilePath: /DOD-WTC3/code/Interface/INicProvider.h
 * @Description:
 * @
 * @Copyright (c) 2026 by AmsAudio Li YuMing, All Rights Reserved.
 */

#pragma once

#include <QString>
#include <QVector>

struct NicInfo {
    QString id;
    QString name;
    QString ipv4;
    QString netmask;
    QString gateway; // 预留：当前不解析平台路由表，恒为空
    QString mac;
    QString broadcast;
};

class INicProvider
{
public:
    virtual ~INicProvider()                        = default;
    virtual QVector<NicInfo> listWiredNics() const = 0;
};