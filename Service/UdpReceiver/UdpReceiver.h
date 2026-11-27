/***
 * @Author: dawn 1964173012@qq.com
 * @Date: 2026-11-27
 * @FilePath: /DOD-D_Serial/Service/UdpReceiver/UdpReceiver.h
 * @Description: UDP 组播监听（独立线程阻塞收包，收到即打印，也可通过信号往外抛）
 * @
 * @Copyright (c) 2026 by AmsAudio Li YuMing, All Rights Reserved.
 */

#pragma once

#include <QByteArray>
#include <QHostAddress>
#include <QString>
#include <QThread>

#include <atomic>

// 用法：
//   UdpReceiver receiver;              // 默认 239.255.254.253:8427
//   receiver.startListening();         // 内部起线程
//   receiver.stop();                   // 退出前调用（析构也会调）
// 端口/组播地址可用环境变量 DOD_UDP_GROUP / DOD_UDP_PORT 覆盖，或调用 setTarget()。
// QML 下拉框选网卡：udpReceiver.localIp = "192.168.3.50"（运行中会自动重启监听）
class UdpReceiver : public QThread
{
    Q_OBJECT

    // 绑定的本机 IPv4（空 = 0.0.0.0 由系统选网卡）
    Q_PROPERTY(QString localIp READ localIp WRITE setLocalIp NOTIFY localIpChanged)
    Q_PROPERTY(QString group READ group NOTIFY targetChanged)
    Q_PROPERTY(int port READ portInt NOTIFY targetChanged)
    Q_PROPERTY(bool listening READ isListening NOTIFY listeningChanged)

public:
    explicit UdpReceiver(QObject* parent = nullptr);
    ~UdpReceiver() override;
    void             setTarget(const QString& group, quint16 port);
    void             setSourceFilter(const QString& sourceIp);
    void             setInterfaceName(const QString& ifaceName);
    Q_INVOKABLE void setLocalIp(const QString& ipv4);
    QString          localIp() const
    {
        return m_localIp;
    }

    QString group() const
    {
        return m_group;
    }
    quint16 port() const
    {
        return m_port;
    }
    int portInt() const
    {
        return m_port;
    }
    QString sourceFilter() const
    {
        return m_sourceFilter;
    }
    bool isListening() const
    {
        return isRunning();
    }

    // 启线程。用这个名字而不是直接 QThread::start()，是为了先清掉上次的停止标志，
    // 避免 start() 后马上 stop() 时线程把标志又改回“运行中”。
    Q_INVOKABLE void startListening();

    // 线程安全：置停止标志，等线程退出
    Q_INVOKABLE void stop();

signals:
    // 收到一包（跨线程 emit，默认队列连接 -> 在接收者线程执行）
    // data 已剥离二进制头，只有业务数据部分，形如：(cid=...),(acn-fctn=ULXD1_RX),(acn-uacn=ULXD4 J50),...
    void datagramReceived(const QByteArray& data, const QHostAddress& sender, quint16 senderPort);

    void localIpChanged();
    void targetChanged();
    void listeningChanged();

protected:
    void run() override;

private:
    QString          m_group;
    quint16          m_port = 8427;
    QString          m_localIp;      // 空 = 0.0.0.0
    QString          m_sourceFilter; // 空 = 收所有源
    QString          m_ifaceName;    // 空 = 自动
    std::atomic_bool m_stopRequested{false};
};
