/**
 * @FilePath: /DOD-D_Serial/Protocol/Ulxd/UlxdClient.cpp
 * @Description: ULX-D 控制连接实现
 */

#include "UlxdClient.h"

#include "Log.h"

#include <QAbstractSocket>
#include <QHostAddress>
#include <QTcpSocket>
#include <QTimer>

namespace
{
    constexpr int kReplyTimeoutMs = 2000;
}

namespace Ulxd
{
    Client::Client(QObject* parent)
        : QObject(parent)
        , m_socket(new QTcpSocket(this))
    {
        // Message 要过信号槽（可能跨线程/排队），注册一下更安全
        qRegisterMetaType<Ulxd::Message>("Ulxd::Message");

        connect(m_socket, &QTcpSocket::connected, this, [this]() {
            LOG_INFO("[Ulxd] 已连接 {}:{}", m_socket->peerAddress().toString(), m_socket->peerPort());
            emit connected();
        });

        connect(m_socket, &QTcpSocket::disconnected, this, [this]() {
            LOG_INFO("[Ulxd] 连接已断开");
            clearPending();
            emit disconnected();
        });

        connect(m_socket, &QTcpSocket::readyRead, this, [this]() {
            const QByteArray     data   = m_socket->readAll();
            const QList<QString> frames = m_reader.feed(data);
            for (const QString& frame : frames) {
                handleFrame(frame);
            }
        });

        connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, [this](QAbstractSocket::SocketError error) {
            Q_UNUSED(error)
            const QString text = m_socket->errorString();
            LOG_WARN("[Ulxd] socket 错误: {}", text);
            emit errorOccurred(text);
        });
    }

    Client::~Client()
    {
        clearPending();
    }

    void Client::connectToDevice(const QString& host, quint16 port)
    {
        if (host.isEmpty()) {
            emit errorOccurred(QStringLiteral("目标地址为空"));
            return;
        }
        if (m_socket->state() != QAbstractSocket::UnconnectedState) {
            m_socket->abort(); // 换设备时先断开旧连接
            m_reader.clear();
        }
        LOG_INFO("[Ulxd] 连接 {}:{}", host, port);
        m_socket->connectToHost(host, port);
    }

    void Client::disconnectFromDevice()
    {
        if (m_socket->state() == QAbstractSocket::UnconnectedState) {
            return;
        }
        m_socket->disconnectFromHost();
    }

    bool Client::isConnected() const
    {
        return m_socket->state() == QAbstractSocket::ConnectedState;
    }

    bool Client::send(const Message& message)
    {
        if (!message.isValid()) {
            LOG_WARN("[Ulxd] 拒绝发送无效消息");
            return false;
        }
        if (!isConnected()) {
            LOG_WARN("[Ulxd] 未连接，丢弃命令: {}", message.toWire());
            return false;
        }

        const QByteArray frame   = message.toWire().toUtf8();
        const qint64     written = m_socket->write(frame);
        if (written != frame.size()) {
            LOG_WARN("[Ulxd] 发送失败: {} ({})", message.toWire(), m_socket->errorString());
            return false;
        }

        LOG_INFO("[Ulxd] -> {}", message.toWire());

        // GET/SET 需要等 REP 回执
        if (message.verb == Verb::Get || message.verb == Verb::Set) {
            Pending pending;
            pending.request = message;

            auto* timer = new QTimer(this);
            timer->setSingleShot(true);
            timer->setInterval(kReplyTimeoutMs);
            connect(timer, &QTimer::timeout, this, [this, timer]() {
                for (int i = 0; i < m_pending.size(); ++i) {
                    if (m_pending.at(i).timer == timer) {
                        const Message request = m_pending.at(i).request;
                        LOG_WARN("[Ulxd] 等待回执超时({}ms): {}", kReplyTimeoutMs, request.toWire());
                        emit requestTimeout(request);
                        removePendingAt(i);
                        break;
                    }
                }
            });
            pending.timer = timer;
            timer->start();

            m_pending.append(pending);
        }
        return true;
    }

    void Client::clearPending()
    {
        while (!m_pending.isEmpty()) {
            removePendingAt(m_pending.size() - 1);
        }
    }

    void Client::removePendingAt(int index)
    {
        if (index < 0 || index >= m_pending.size()) {
            return;
        }
        if (m_pending.at(index).timer != nullptr) {
            m_pending.at(index).timer->stop();
            m_pending.at(index).timer->deleteLater();
        }
        m_pending.removeAt(index);
    }

    bool Client::takePending(const Message& reply, Message* request)
    {
        // 被动上报的 REP（例如面板改参数）没有对应请求，直接放过
        for (int i = 0; i < m_pending.size(); ++i) {
            if (m_pending.at(i).request.isReplyTo(reply.parameter, reply.channel)) {
                if (request != nullptr) {
                    *request = m_pending.at(i).request;
                }
                removePendingAt(i);
                return true;
            }
        }
        return false;
    }

    void Client::handleFrame(const QString& frame)
    {
        bool          ok      = false;
        const Message message = Message::parse(frame, &ok);
        if (!ok) {
            LOG_WARN("[Ulxd] 无法解析帧: {}", frame);
            return;
        }

        LOG_INFO("[Ulxd] <- {}", frame);
        emit messageReceived(message);

        if (message.verb == Verb::Err && message.parameter.isEmpty()) {
            // 设备拒绝时只回一条 < REP ERR >（不带通道/参数），无法按参数匹配到请求。
            // 归到最早的那个待回执请求上，否则调用方只能干等到超时，看不出是“被拒绝”。
            if (m_pending.isEmpty()) {
                LOG_WARN("[Ulxd] 收到 {}，但当前没有待回执请求", frame);
            }
            else {
                const Message rejected = m_pending.first().request;
                removePendingAt(0);
                LOG_WARN("[Ulxd] 设备拒绝: {}（可能是格式/范围/频段不匹配）", rejected.toWire());
                emit requestRejected(rejected);
                emit replyReceived(message);
            }
            return;
        }

        if (message.verb == Verb::Rep || message.verb == Verb::Err) {
            if (takePending(message, nullptr)) {
                emit replyReceived(message);
            }
            // 值本身是错误码（252..255 / 65532..65535）时提示一下，业务层好定位
            for (const QString& arg : message.args) {
                if (isErrorValue(arg)) {
                    LOG_WARN("[Ulxd] {} {} = {}", message.parameter, message.channel, describeError(arg));
                }
            }
        }
    }
} // namespace Ulxd
