/***
 * @Author: dawn 1964173012@qq.com
 * @Date: 2026-11-26 17:05:56
 * @LastEditors: dawn 1964173012@qq.com
 * @LastEditTime: 2026-11-26 17:06:12
 * @FilePath: /DOD-D_Serial/Service/NicSearch/NicSearch.h
 * @Description:
 * @
 * @Copyright (c) 2026 by AmsAudio Li YuMing, All Rights Reserved.
 */

#pragma once

#include <QObject>

class NicSearch : public QObject
{
    Q_OBJECT
public:
    NicSearch(QObject* parent = nullptr);
    ~NicSearch();
};