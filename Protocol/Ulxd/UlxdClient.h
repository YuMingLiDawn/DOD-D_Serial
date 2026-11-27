/**
 * @FilePath: /DOD-D_Serial/Protocol/Ulxd/UlxdClient.h
 * @Description: ULX-D 控制连接（TCP 2202）
 *
 * 职责边界：
 *   - 只做「连接 / 发帧 / 收帧 / 回执超时匹配 / 日志」
 *   - 不解析业务含义（那是 Biz/Model 层的事）
 *   - 命令只接受 Ulxd::Message，调用方通过 UlxdCommands 构造，避免手拼字符串
 */

#pragma once

#include "UlxdFrameReader.h"
#include "UlxdMessage.h"

#include <QObject>
#include <QVector>

class QTcpSocket;
class QTimer;

namespace Ulxd
{
    class Client : public QObject
    {
        Q_OBJECT

    public:
        explicit Client(QObject* parent = nullptr);
        ~Client() override;

        Q_INVOKABLE void connectToDevice(const QString& host, quint16 port = kDefaultTcpPort);
        Q_INVOKABLE void disconnectFromDevice();
        bool             isConnected() const;

        // 发送一条命令；GET/SET 自动登记回执，收到对应 REP 时发 replyReceived
        bool send(const Message& message);

        int pendingCount() const
        {
            return m_pending.size();
        }
        void clearPending();

    signals:
        void connected();
        void disconnected();
        void messageReceived(const Ulxd::Message& message); // 所有收到的帧（含主动上报的 REP/SAMPLE）
        void replyReceived(const Ulxd::Message& reply);     // 与你发出的请求匹配上的 REP
        void requestTimeout(const Ulxd::Message& request);  // 超时没等到 REP
        void requestRejected(const Ulxd::Message& request); // 设备只回了 < REP ERR >（没带参数，无法按参数匹配）
        void errorOccurred(const QString& error);

    private:
        struct Pending {
            Message request;
            QTimer* timer = nullptr;
        };

        void handleFrame(const QString& frame);
        bool takePending(const Message& reply, Message* request);
        void removePendingAt(int index);

        QTcpSocket*      m_socket = nullptr;
        FrameReader      m_reader;
        QVector<Pending> m_pending;
    };
} // namespace Ulxd
