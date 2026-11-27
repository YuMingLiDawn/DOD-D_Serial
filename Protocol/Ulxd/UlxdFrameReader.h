/**
 * @FilePath: /DOD-D_Serial/Protocol/Ulxd/UlxdFrameReader.h
 * @Description: TCP 字节流 → 帧 的切分器（处理粘包/半包/杂字节）
 *
 * TCP 是字节流，设备可能把多条 REP 合在一起发，也可能一条被拆成两段。
 * 这个类只负责按 < ... > 边界切帧，与业务无关，可单独测试。
 */

#pragma once

#include <QByteArray>
#include <QList>
#include <QString>

namespace Ulxd
{
    class FrameReader
    {
    public:
        // 追加一段收到的数据，返回本次能切出的完整帧（含尖括号）
        QList<QString> feed(const QByteArray& data)
        {
            QList<QString> frames;
            m_buffer.append(data);

            while (true) {
                const int start = m_buffer.indexOf('<');
                if (start < 0) {
                    // 没有帧头，丢弃杂字节
                    if (!m_buffer.isEmpty()) {
                        m_buffer.clear();
                    }
                    break;
                }
                if (start > 0) {
                    m_buffer.remove(0, start); // 丢掉 < 之前的垃圾
                }

                const int end = m_buffer.indexOf('>');
                if (end < 0) {
                    break; // 半包，等后续数据
                }

                frames << QString::fromUtf8(m_buffer.left(end + 1));
                m_buffer.remove(0, end + 1);
            }

            // 防止异常流把缓冲撑爆
            if (m_buffer.size() > 64 * 1024) {
                m_buffer.clear();
            }
            return frames;
        }

        void clear()
        {
            m_buffer.clear();
        }

        int buffered() const
        {
            return m_buffer.size();
        }

    private:
        QByteArray m_buffer;
    };
} // namespace Ulxd
