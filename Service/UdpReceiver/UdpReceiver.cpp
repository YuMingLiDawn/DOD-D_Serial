/***
 * @Author: dawn 1964173012@qq.com
 * @Date: 2026-11-27
 * @FilePath: /DOD-D_Serial/Service/UdpReceiver/UdpReceiver.cpp
 * @Description: UDP 组播监听实现
 * @
 * @Copyright (c) 2026 by AmsAudio Li YuMing, All Rights Reserved.
 */

#include "UdpReceiver.h"

#include "Log.h"
#include "UdpTransportQt.h" // 现成的收发组件（Infrastructure）

#include <QHostAddress>
#include <QElapsedTimer>
#include <QNetworkAddressEntry>
#include <QNetworkInterface>

namespace
{
    // 十六进制 + 可读文本预览，排查协议最直观
    QString payloadPreview(const QByteArray& data, int maxBytes = 64)
    {
        const int n = qMin(data.size(), maxBytes);

        QString hex;
        hex.reserve(n * 3);
        QString text;
        text.reserve(n);

        for (int i = 0; i < n; ++i) {
            const auto byte = static_cast<quint8>(data.at(i));
            hex += QStringLiteral("%1").arg(byte, 2, 16, QLatin1Char('0')).toUpper();
            hex += QLatin1Char(' ');
            text += (byte >= 0x20 && byte < 0x7f) ? QLatin1Char(static_cast<char>(byte)) : QLatin1Char('.');
        }

        QString out = hex.trimmed() + QStringLiteral("  |") + text + QLatin1Char('|');
        if (data.size() > maxBytes) {
            out += QStringLiteral(" …(共 %1 字节)").arg(data.size());
        }
        return out;
    }

    // Shure ULX-D 发现报文：前面是二进制头（末 4 字节为大端 data 长度），
    // 真正的业务数据是 "(key=value),(key=value)..." 的 ASCII 文本，末尾补一个 '\0'。
    // 实测一包 319 字节 = 20 字节头 + 299 字节 data（开头就是 '('）。
    QByteArray extractPayload(const QByteArray& datagram)
    {
        const int start = datagram.indexOf('(');
        if (start < 0) {
            return datagram; // 不是这个格式，原样返回别丢数据
        }
        QByteArray payload = datagram.mid(start);
        while (!payload.isEmpty() && payload.endsWith('\0')) {
            payload.chop(1);
        }
        return payload;
    }

    // 根据本机 IP 找到它所属的网卡名（给 joinMulticastGroup 用）
    QString interfaceNameForIp(const QString& ipv4)
    {
        if (ipv4.isEmpty()) {
            return {};
        }
        for (const QNetworkInterface& iface : QNetworkInterface::allInterfaces()) {
            for (const QNetworkAddressEntry& entry : iface.addressEntries()) {
                if (entry.ip().toString() == ipv4) {
                    return iface.name();
                }
            }
        }
        return {};
    }
} // namespace

UdpReceiver::UdpReceiver(QObject* parent)
    : QThread(parent)
{
    // 默认值可用环境变量覆盖，方便换现场/换协议不改代码
    m_group        = qEnvironmentVariable("DOD_UDP_GROUP", QStringLiteral("239.255.254.253"));
    m_sourceFilter = qEnvironmentVariable("DOD_UDP_SRC");      // 例：DOD_UDP_SRC=192.168.3.123
    m_ifaceName    = qEnvironmentVariable("DOD_UDP_IFACE");    // 例：DOD_UDP_IFACE=enp1s0
    m_localIp      = qEnvironmentVariable("DOD_UDP_LOCAL_IP"); // 例：DOD_UDP_LOCAL_IP=192.168.3.50

    bool      ok      = false;
    const int envPort = qEnvironmentVariableIntValue("DOD_UDP_PORT", &ok);
    m_port            = ok && envPort > 0 && envPort <= 65535 ? static_cast<quint16>(envPort) : static_cast<quint16>(8427);
}

UdpReceiver::~UdpReceiver()
{
    stop();
}

void UdpReceiver::setTarget(const QString& group, quint16 port)
{
    if (isRunning()) {
        LOG_WARN("[UdpReceiver] 正在监听中，忽略 setTarget({}:{})", group, port);
        return;
    }
    m_group = group;
    m_port  = port;
    emit targetChanged();
}

void UdpReceiver::setSourceFilter(const QString& sourceIp)
{
    if (isRunning()) {
        LOG_WARN("[UdpReceiver] 正在监听中，忽略 setSourceFilter({})", sourceIp);
        return;
    }
    m_sourceFilter = sourceIp;
}

void UdpReceiver::setInterfaceName(const QString& ifaceName)
{
    if (isRunning()) {
        LOG_WARN("[UdpReceiver] 正在监听中，忽略 setInterfaceName({})", ifaceName);
        return;
    }
    m_ifaceName = ifaceName;
}

void UdpReceiver::setLocalIp(const QString& ipv4)
{
    if (ipv4 == m_localIp) {
        return;
    }
    m_localIp = ipv4;
    emit localIpChanged();
    LOG_INFO("[UdpReceiver] 绑定 IP 改为 {}", m_localIp.isEmpty() ? QStringLiteral("<any>") : m_localIp);

    // 运行中改 IP：直接重启线程，按新网卡重新 bind + join 组播
    if (isRunning()) {
        stop();
        startListening();
    }
}

void UdpReceiver::startListening()
{
    if (isRunning()) {
        return;
    }
    m_stopRequested = false;
    start();
    emit listeningChanged();
}

void UdpReceiver::stop()
{
    m_stopRequested = true;
    if (isRunning()) {
        // waitForReadyRead(200ms) 会周期性返回，2 秒足够
        if (!wait(2000)) {
            LOG_WARN("[UdpReceiver] 线程未在 2s 内退出，强制终止");
            terminate();
            wait();
        }
    }
    emit listeningChanged();
}

void UdpReceiver::run()
{
    // transport 必须在本线程内创建：它内部的 QUdpSocket 归属本线程，
    // 才能安全地用 waitForReadyRead 阻塞收包。
    UdpTransportQt transport;

    // 绑定的本机 IP：空 = 0.0.0.0（系统选网卡）；非空时把收包网卡钉死在该网卡上
    if (!transport.bind(m_localIp, m_port)) {
        LOG_ERROR("[UdpReceiver] bind 失败 localIp={} port={}", m_localIp.isEmpty() ? QStringLiteral("<any>") : m_localIp, m_port);
        return;
    }
    if (!m_localIp.isEmpty()) {
        transport.setLocalAddress(m_localIp); // 之后若用该 transport 发包，出口也是这块网卡
    }

    // 加入组播的网卡：显式指定 > 按 localIp 推导 > 交给组件遍历所有网卡
    const QString joinIface = m_ifaceName.isEmpty() ? interfaceNameForIp(m_localIp) : m_ifaceName;
    if (!transport.joinMulticastGroup(m_group, m_port, joinIface)) {
        LOG_ERROR("[UdpReceiver] 加入组播失败 group={} port={} iface={}", m_group, m_port, joinIface);
        return;
    }

    LOG_INFO("[UdpReceiver] 开始监听组播 {}:{}（线程 {}）", m_group, m_port, reinterpret_cast<quintptr>(QThread::currentThreadId()));

    // 一包的处理逻辑：过滤源地址 -> 剥离头部取 data -> 打日志 -> 抛信号
    const auto handleOne = [this](const QByteArray& raw, const QString& senderIp, quint16 senderPort) {
        if (!m_sourceFilter.isEmpty() && senderIp != m_sourceFilter) {
            // 不静默丢弃：排查“明明有包却没打印”时这里最有用
            LOG_DEBUG("[UdpReceiver] 丢弃非目标来源 {}:{}（期望 {}）", senderIp, senderPort, m_sourceFilter);
            return;
        }

        const QByteArray payload = extractPayload(raw);
        if (payload.isEmpty()) {
            LOG_WARN("[UdpReceiver] 收到空数据  来自 {}:{}", senderIp, senderPort);
            return;
        }

        LOG_INFO("[UdpReceiver] data({} 字节) 来自 {}:{}\n  {}", payload.size(), senderIp, senderPort, QString::fromLatin1(payload));
        LOG_TRACE("[UdpReceiver] 原始报文 {} 字节: {}", raw.size(), payloadPreview(raw, 32));

        emit datagramReceived(payload, QHostAddress(senderIp), senderPort);
    };

    // 长时间收不到数据时提醒一次/分钟，便于区分“没在听”和“听了但没包”
    QElapsedTimer idleTimer;
    idleTimer.start();

    while (!m_stopRequested.load()) {
        QByteArray data;
        QString    senderIp;
        quint16    senderPort = 0;

        // 阻塞最多 200ms，超时只是为了能及时响应 stop()
        if (!transport.receiveFrom(m_port, data, senderIp, senderPort, 200)) {
            if (idleTimer.elapsed() > 60000) {
                LOG_WARN("[UdpReceiver] 已 60s 未收到任何数据 group={} port={} localIp={} iface={}，检查网段/防火墙/IGMP", m_group, m_port, m_localIp.isEmpty() ? QStringLiteral("<any>") : m_localIp,
                         joinIface);
                idleTimer.restart();
            }
            continue;
        }

        idleTimer.restart();
        handleOne(data, senderIp, senderPort);

        // 同一批往往有多包，一次排空，避免下一轮还排在队列里
        transport.drainAllFrom(m_port, handleOne);
    }

    LOG_INFO("[UdpReceiver] 已停止监听 {}:{}", m_group, m_port);
}
