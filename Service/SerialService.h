#pragma once

#include <QObject>
#include <QString>

// 全局服务示例（单例）：QML 里像普通对象一样用 serialService.xxx
// 真实项目里这里换成串口/网络/协议栈等逻辑，QML 只依赖它的属性和槽。
class SerialService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)
    Q_PROPERTY(QString lastMessage READ lastMessage NOTIFY lastMessageChanged)

public:
    static SerialService& instance();

    bool isConnected() const
    {
        return m_connected;
    }
    QString lastMessage() const
    {
        return m_lastMessage;
    }

    // 给 QML 调用的接口（Q_INVOKABLE 等价于 public slot）
    Q_INVOKABLE void connectPort(const QString& portName);
    Q_INVOKABLE void disconnectPort();
    Q_INVOKABLE void sendText(const QString& text);

signals:
    void connectedChanged();
    void lastMessageChanged();

private:
    explicit SerialService(QObject* parent = nullptr);

    void setConnected(bool connected);
    void setLastMessage(const QString& message);

    bool    m_connected = false;
    QString m_lastMessage;
};
