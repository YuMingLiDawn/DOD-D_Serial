/**
 * @FilePath: /DOD-D_Serial/Protocol/Ulxd/UlxdCommands.cpp
 * @Description: 语义化命令工厂实现（所有格式细节集中在这里）
 */

#include "UlxdCommands.h"

#include "Log.h"

#include <cmath>

namespace
{
    using namespace Ulxd;

    // 统一构造：verb + 通道 + 参数 + 参数值
    Message make(Verb verb, int channel, const QString& parameter, const QStringList& args = {}, const QVector<bool>& quoted = QVector<bool>())
    {
        Message message;
        message.verb       = verb;
        message.channel    = channel;
        message.parameter  = parameter;
        message.args       = args;
        message.quotedArgs = quoted;
        // 补齐 quoted 长度，避免越界判断每处都写
        while (message.quotedArgs.size() < message.args.size()) {
            message.quotedArgs << false;
        }
        return message;
    }

    // 通道号：0 = 全部；接收机 1..4，充电器 1..8
    int channelOrDefault(int channel)
    {
        if (channel < 0) {
            LOG_WARN("[Ulxd] 通道号非法({})，回退为 0（全部通道）", channel);
            return kAllChannels;
        }
        if (channel > 8) {
            LOG_WARN("[Ulxd] 通道号超出范围({})，截断为 8", channel);
            return 8;
        }
        return channel;
    }

    int clamp(int value, int lo, int hi, const char* what)
    {
        if (value < lo || value > hi) {
            const int clamped = qBound(lo, value, hi);
            LOG_WARN("[Ulxd] {}={} 越界，截断为 {}", what, value, clamped);
            return clamped;
        }
        return value;
    }

    // 固定宽度的 ASCII 数字（不足补 0）：000 / 00000
    QString ascii(int value, int width)
    {
        return QStringLiteral("%1").arg(value, width, 10, QLatin1Char('0'));
    }

    // 字符串值：去掉换行，截到 8 字符（协议上限），空值不合法
    QString fixedText(const QString& text, const char* what)
    {
        QString value = text.trimmed();
        value.replace(QLatin1Char('\n'), QLatin1Char(' '));
        if (value.isEmpty()) {
            LOG_WARN("[Ulxd] {} 为空，设备可能拒绝；建议使用 1..8 个字符", what);
        }
        if (value.size() > 8) {
            LOG_WARN("[Ulxd] {} 超长被截断到 8 字符: {}", what, value);
            value = value.left(8);
        }
        return value;
    }

    Message onOffCommand(Verb verb, int channel, const QString& parameter, bool on)
    {
        return make(verb, channel, parameter, {on ? QStringLiteral("ON") : QStringLiteral("OFF")});
    }
} // namespace

namespace Ulxd
{
    namespace Commands
    {
        // ---------------- 通用 ----------------
        Message getAll(int channel)
        {
            return make(Verb::Get, channelOrDefault(channel), QStringLiteral("ALL"));
        }

        Message get(int channel, const QString& parameter)
        {
            return make(Verb::Get, channelOrDefault(channel), parameter.toUpper());
        }

        Message set(int channel, const QString& parameter, const QStringList& args, const QVector<bool>& quoted)
        {
            return make(Verb::Set, channelOrDefault(channel), parameter.toUpper(), args, quoted);
        }

        // ---------------- 设备级 ----------------
        Message getFirmwareVersion()
        {
            return make(Verb::Get, kNoChannel, QStringLiteral("FW_VER"));
        }

        Message getDeviceId()
        {
            return make(Verb::Get, kNoChannel, QStringLiteral("DEVICE_ID"));
        }

        Message setDeviceId(const QString& id)
        {
            return make(Verb::Set, kNoChannel, QStringLiteral("DEVICE_ID"), {fixedText(id, "DEVICE_ID")}, {true});
        }

        Message getHighDensity()
        {
            return make(Verb::Get, kNoChannel, QStringLiteral("HIGH_DENSITY"));
        }

        Message setHighDensity(bool on)
        {
            return onOffCommand(Verb::Set, kNoChannel, QStringLiteral("HIGH_DENSITY"), on);
        }

        Message getFlash()
        {
            return make(Verb::Get, kNoChannel, QStringLiteral("FLASH"));
        }

        Message setFlash(bool on)
        {
            return onOffCommand(Verb::Set, kNoChannel, QStringLiteral("FLASH"), on);
        }

        Message regenerateEncryptionKey()
        {
            return make(Verb::Set, kNoChannel, QStringLiteral("ENCRYPTION_REGENERATE_KEY"), {QStringLiteral("REQUESTED")});
        }

        // ---------------- 通道级 ----------------
        Message getChannelName(int channel)
        {
            return make(Verb::Get, channelOrDefault(channel), QStringLiteral("CHAN_NAME"));
        }

        Message setChannelName(int channel, const QString& name)
        {
            return make(Verb::Set, channelOrDefault(channel), QStringLiteral("CHAN_NAME"), {fixedText(name, "CHAN_NAME")}, {true});
        }

        Message getAudioMute(int channel)
        {
            return make(Verb::Get, channelOrDefault(channel), QStringLiteral("AUDIO_MUTE"));
        }

        Message setAudioMute(int channel, bool muted)
        {
            return onOffCommand(Verb::Set, channelOrDefault(channel), QStringLiteral("AUDIO_MUTE"), muted);
        }

        Message toggleAudioMute(int channel)
        {
            return make(Verb::Set, channelOrDefault(channel), QStringLiteral("AUDIO_MUTE"), {QStringLiteral("TOGGLE")});
        }

        Message getAudioGain(int channel)
        {
            return make(Verb::Get, channelOrDefault(channel), QStringLiteral("AUDIO_GAIN"));
        }

        Message setAudioGain(int channel, int gain)
        {
            return make(Verb::Set, channelOrDefault(channel), QStringLiteral("AUDIO_GAIN"), {ascii(clamp(gain, 0, 60, "AUDIO_GAIN"), 3)});
        }

        Message increaseAudioGain(int channel, int dB)
        {
            return make(Verb::Set, channelOrDefault(channel), QStringLiteral("AUDIO_GAIN"), {QStringLiteral("INC"), QString::number(clamp(dB, 1, 60, "gain INC"))});
        }

        Message decreaseAudioGain(int channel, int dB)
        {
            return make(Verb::Set, channelOrDefault(channel), QStringLiteral("AUDIO_GAIN"), {QStringLiteral("DEC"), QString::number(clamp(dB, 1, 60, "gain DEC"))});
        }

        Message getFrequency(int channel)
        {
            return make(Verb::Get, channelOrDefault(channel), QStringLiteral("FREQUENCY"));
        }

        Message setFrequency(int channel, int khz)
        {
            // 实机（ULXD4 固件 2.10.0.0）接受/回读的都是 6 位整数 kHz：550000 = 550.000MHz。
            // 注意：带小数点的 550.000 会被设备直接回 < REP ERR >。
            if (khz < kMinFrequencyKhz || khz > kMaxFrequencyKhz) {
                LOG_WARN("[Ulxd] FREQUENCY={}kHz 不在协议范围 {}~{}kHz 内（设备还可能因频段不符返回 ERR）", khz, kMinFrequencyKhz, kMaxFrequencyKhz);
            }
            return make(Verb::Set, channelOrDefault(channel), QStringLiteral("FREQUENCY"), {ascii(khz, 6)});
        }

        int frequencyToKhz(const QString& rawValue)
        {
            const QString text = rawValue.trimmed();
            if (text.contains(QLatin1Char('.'))) {
                // 旧文档格式 yyy.yyy（MHz）→ kHz
                const double mhz = text.toDouble();
                return static_cast<int>(std::lround(mhz * 1000.0));
            }
            return text.toInt();
        }

        double frequencyToMhz(const QString& rawValue)
        {
            return frequencyToKhz(rawValue) / 1000.0;
        }

        Message getGroupChannel(int channel)
        {
            return make(Verb::Get, channelOrDefault(channel), QStringLiteral("GROUP_CHAN"));
        }

        Message setGroupChannel(int channel, int group, int channelInGroup)
        {
            // 注意：QString::arg(int,int,...) 是「宽度/进制」重载，多参数必须用 QString::number 或链式 arg
            const QString value = QStringLiteral("%1,%2").arg(QString::number(clamp(group, 0, 99, "GROUP"))).arg(QString::number(channelInGroup));
            return make(Verb::Set, channelOrDefault(channel), QStringLiteral("GROUP_CHAN"), {value});
        }

        Message setMeteringRate(int channel, int intervalMs)
        {
            // 0 = 关闭；其它最小 100ms，最大 99999ms
            const int value = intervalMs == 0 ? 0 : clamp(intervalMs, 100, 99999, "METER_RATE");
            return make(Verb::Set, channelOrDefault(channel), QStringLiteral("METER_RATE"), {ascii(value, 5)});
        }

        // ---------------- 电池 / 发射机 ----------------
        Message getBatteryCharge(int channel)
        {
            return make(Verb::Get, channelOrDefault(channel), QStringLiteral("BATT_CHARGE"));
        }

        Message getBatteryBars(int channel)
        {
            return make(Verb::Get, channelOrDefault(channel), QStringLiteral("BATT_BARS"));
        }

        Message getBatteryHealth(int channel)
        {
            return make(Verb::Get, channelOrDefault(channel), QStringLiteral("BATT_HEALTH"));
        }

        Message getBatteryCycle(int channel)
        {
            return make(Verb::Get, channelOrDefault(channel), QStringLiteral("BATT_CYCLE"));
        }

        Message getBatteryTempC(int channel)
        {
            return make(Verb::Get, channelOrDefault(channel), QStringLiteral("BATT_TEMP_C"));
        }

        Message getBatteryType(int channel)
        {
            return make(Verb::Get, channelOrDefault(channel), QStringLiteral("BATT_TYPE"));
        }

        Message getTransmitterType(int channel)
        {
            return make(Verb::Get, channelOrDefault(channel), QStringLiteral("TX_TYPE"));
        }

        Message getTxOffset(int channel)
        {
            return make(Verb::Get, channelOrDefault(channel), QStringLiteral("TX_OFFSET"));
        }

        Message getTxRfPower(int channel)
        {
            return make(Verb::Get, channelOrDefault(channel), QStringLiteral("TX_RF_PWR"));
        }
    } // namespace Commands
} // namespace Ulxd
