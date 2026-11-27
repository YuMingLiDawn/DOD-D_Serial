/**
 * @FilePath: /DOD-D_Serial/Protocol/Ulxd/UlxdMessage.h
 * @Description: 统一消息模型 —— 收发的唯一数据结构，所有命令都最终变成它
 *
 * 设计原则：
 *   1. 一个结构同时表示命令（GET/SET）和响应（REP/SAMPLE/ERR），便于对称处理；
 *   2. 只做序列化/反序列化，不碰网络、不碰 UI（可单独写单测）；
 *   3. 业务侧永远不手拼字符串，统一走 UlxdCommands + UlxdCodec。
 */

#pragma once

#include "UlxdTypes.h"

#include <QMetaType>
#include <QString>
#include <QStringList>
#include <QVector>

namespace Ulxd
{
    struct Message {
        Verb          verb    = Verb::Unknown;
        int           channel = kNoChannel; // 设备级命令为 kNoChannel；kAllChannels(0) 表示全部通道
        QString       parameter;            // "AUDIO_GAIN" / "CHAN_NAME" / "SAMPLE"
        QStringList   args;                 // 参数原文，字符串值已去掉花括号
        QVector<bool> quotedArgs;           // 与 args 对齐：true 表示该参数在线上要包 {}
        QString       raw;                  // 原始帧，便于日志排查

        bool isValid() const
        {
            return verb != Verb::Unknown && !parameter.isEmpty();
        }

        bool isReplyTo(const QString& otherParameter, int otherChannel) const
        {
            const bool channelOk = (otherChannel == channel) || (otherChannel == kAllChannels) || (channel == kAllChannels);
            return parameter == otherParameter && channelOk;
        }

        QString arg(int index, const QString& defaultValue = QString()) const
        {
            return index >= 0 && index < args.size() ? args.at(index) : defaultValue;
        }

        int  argInt(int index, int defaultValue = 0) const;
        bool argBool(int index, bool defaultValue = false) const; // ON/OFF/YES/NO

        // "< SET 1 AUDIO_GAIN 020 >"
        QString toWire() const;

        // 解析一帧（可带不带尖括号）；失败返回 isValid()==false
        static Message parse(const QString& frame, bool* ok = nullptr);
    };
} // namespace Ulxd

Q_DECLARE_METATYPE(Ulxd::Message)
