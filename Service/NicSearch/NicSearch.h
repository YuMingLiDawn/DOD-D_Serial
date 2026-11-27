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
#include <QStringList>
#include <QVariantList>

#include <memory>

class INicProvider;
class QTimer;
struct NicInfo;

class NicSearch : public QObject
{
    Q_OBJECT

public:
    static NicSearch& instance();

public:
    QVariantList nics() const
    {
        return m_nics;
    }

    int count() const
    {
        return m_nics.size();
    }

    bool isWatching() const;
    int  intervalMs() const
    {
        return m_intervalMs;
    }
    void setIntervalMs(int intervalMs);

    QVariantList listWiredNics() const;
    QVector<NicInfo> listWiredNicsRaw() const;

    void start();   // 开始定时检测
    void stop();    // 停止定时检测（已拿到的数据保留）
    void refresh(); // 立刻检测一次

signals:
    void nicsChanged();
    void watchingChanged();
    void intervalMsChanged();

private:
    explicit NicSearch(QObject* parent = nullptr);
    ~NicSearch() override;

private:
    // 扫一次，与上次快照比对；有变化才改数据、发信号
    void scan();

    std::unique_ptr<INicProvider> m_provider;
    QTimer*                       m_timer = nullptr;
    QVariantList                  m_nics;
    QStringList                   m_snapshot; // 上次快照：name|ipv4|netmask|mac|broadcast
    int                           m_intervalMs = 2000;
};
