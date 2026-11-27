/**
 * @FilePath: /DOD-D_Serial/Protocol/Ulxd/UlxdCommands.h
 * @Description: 语义化命令工厂 —— 业务侧唯一应该使用的「发命令」入口
 *
 * 约定：
 *   1. 每个函数返回 Ulxd::Message，调用方只关心语义（gain/dB/频率），不关心格式；
 *   2. 取值校验、补零（yyy/yyyyy）、字符串补 8 位等「格式细节」全部集中在这里；
 *   3. 新增命令 = 在本文件加一个函数（+ .cpp 实现），其它地方不用改。
 *
 * 设备侧命令参数顺序参考：Shure《Command Strings for ULX-D Receivers》
 */

#pragma once

#include "UlxdMessage.h"

namespace Ulxd
{
    namespace Commands
    {
        // ---------- 通用 ----------
        Message getAll(int channel = kAllChannels);
        Message get(int channel, const QString& parameter);
        Message set(int channel, const QString& parameter, const QStringList& args = {}, const QVector<bool>& quoted = {});

        // ---------- 设备级（无通道号） ----------
        Message getFirmwareVersion();
        Message getDeviceId();
        Message setDeviceId(const QString& id); // 1..8 字符，自动补空格到 8
        Message getHighDensity();
        Message setHighDensity(bool on);
        Message getFlash();
        Message setFlash(bool on);
        Message regenerateEncryptionKey();

        // ---------- 通道级 ----------
        Message getChannelName(int channel);
        Message setChannelName(int channel, const QString& name); // 1..8 字符

        Message getAudioMute(int channel);
        Message setAudioMute(int channel, bool muted);
        Message toggleAudioMute(int channel);

        Message getAudioGain(int channel);
        Message setAudioGain(int channel, int gain);    // 000..060（显示值 = gain - 18）
        Message increaseAudioGain(int channel, int dB); // 1..60
        Message decreaseAudioGain(int channel, int dB); // 1..60

        Message getFrequency(int channel);
        Message setFrequency(int channel, int khz);      // 直接传 kHz：550000 → < SET 1 FREQUENCY 550000 >
        int     frequencyToKhz(const QString& rawValue); // 回读值 632700 / 632.700 都转成 632700
        double  frequencyToMhz(const QString& rawValue); // 回读值 632700 / 632.700 都转成 632.7

        Message getGroupChannel(int channel);
        Message setGroupChannel(int channel, int group, int channelInGroup);

        Message setMeteringRate(int channel, int intervalMs); // 0=关闭，否则 100..99999

        // ---------- 电池 / 发射机 ----------
        Message getBatteryCharge(int channel); // 000..100
        Message getBatteryBars(int channel);   // 000..005
        Message getBatteryHealth(int channel); // 000..100
        Message getBatteryCycle(int channel);  // 5 位
        Message getBatteryTempC(int channel);  // 温度 + 40
        Message getBatteryType(int channel);
        Message getTransmitterType(int channel);
        Message getTxOffset(int channel);
        Message getTxRfPower(int channel);
    } // namespace Commands
} // namespace Ulxd
