/***
 * @FilePath: /DOD-D_Serial/Protocol/Ulxd/UlxdTypes.h
 * @Description: ULX-D 网络字符串协议 —— 基础类型/常量（无 Qt GUI、无 IO，方便单测）
 *
 * 协议要点（Shure ULX-D Network String Commands, TCP 2202, ASCII）：
 *   帧格式：< VERB [x] PARAM [args] >
 *     VERB  : GET | SET | REP | ERR | SAMPLE
 *     x     : 通道号 0..4（0 = 全部通道；单通道接收机固定为 1）
 *             设备级命令没有 x（如 GET DEVICE_ID / SET FLASH ON）
 *     PARAM : 参数字符串（CHAN_NAME / AUDIO_GAIN / ...）
 *     args  : 字符串值用花括号 {name}；数值用固定宽度的 ASCII 数字
 * @Copyright (c) 2026 by AmsAudio Li YuMing, All Rights Reserved.
 */

#pragma once

#include <QString>

namespace Ulxd
{
    // 通道号：0 表示“全部通道”（多通道接收机），设备级命令用 kNoChannel
    constexpr int kAllChannels = 0;
    constexpr int kNoChannel   = -1;

    constexpr quint16 kDefaultTcpPort = 2202;

    // FREQUENCY 的合法范围，单位 kHz；具体机器还受自身频段限制，越界设备会回 < REP ERR >
    constexpr int kMinFrequencyKhz = 470000; // 470.000MHz
    constexpr int kMaxFrequencyKhz = 952000; // 952.000MHz

    enum class Verb { Get, Set, Rep, Err, Sample, Unknown };

    QString verbToString(Verb verb);
    Verb    verbFromString(const QString& text);

    // 3 位数值的错误码：252..255；5 位数值的错误码：65532..65535
    // 含义：被控设备（话筒）不可用 —— 关机、未响应等
    bool    isErrorValue(const QString& asciiValue);
    QString describeError(const QString& asciiValue); // "65535" -> "65535（设备不可用/未响应）"
} // namespace Ulxd
