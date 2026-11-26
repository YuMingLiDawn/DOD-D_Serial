#include "SerialService.h"

#include "Log.h"

SerialService& SerialService::instance()
{
    // 单例随进程存活，不需要父对象
    static SerialService service;
    return service;
}

SerialService::SerialService(QObject* parent)
    : QObject(parent)
{}

void SerialService::connectPort(const QString& portName)
{
    LOG_INFO("[SerialService] connect: {}", portName);
    setConnected(true);
    setLastMessage(QStringLiteral("已连接 %1").arg(portName));
}

void SerialService::disconnectPort()
{
    LOG_INFO("[SerialService] disconnect");
    setConnected(false);
    setLastMessage(QStringLiteral("已断开"));
}

void SerialService::sendText(const QString& text)
{
    // 真实实现：把数据丢给 IO 线程，结果再用信号回主线程
    LOG_INFO("[SerialService] send: {}", text);
    setLastMessage(QStringLiteral("发送: %1").arg(text));
}

void SerialService::setConnected(bool connected)
{
    if (m_connected == connected) {
        return;
    }
    m_connected = connected;
    emit connectedChanged();
}

void SerialService::setLastMessage(const QString& message)
{
    if (m_lastMessage == message) {
        return;
    }
    m_lastMessage = message;
    emit lastMessageChanged();
}
