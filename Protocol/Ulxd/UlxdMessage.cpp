/**
 * @FilePath: /DOD-D_Serial/Protocol/Ulxd/UlxdMessage.cpp
 * @Description: 统一消息模型的解析与序列化
 */

#include "UlxdMessage.h"

#include <QStringList>

namespace
{
    // 按空白切分，但 {...} 内的空格属于字符串值，不能切
    QStringList tokenize(const QString& text)
    {
        QStringList tokens;
        QString     current;
        bool        inBraces = false;

        for (const QChar ch : text) {
            if (ch == QLatin1Char('{')) {
                inBraces = true;
                current.append(ch);
                continue;
            }
            if (ch == QLatin1Char('}')) {
                inBraces = false;
                current.append(ch);
                continue;
            }
            if (!inBraces && ch.isSpace()) {
                if (!current.isEmpty()) {
                    tokens << current;
                    current.clear();
                }
                continue;
            }
            current.append(ch);
        }
        if (!current.isEmpty()) {
            tokens << current;
        }
        return tokens;
    }

    bool looksLikeChannel(const QString& token)
    {
        if (token.isEmpty() || token.size() > 1) {
            return token == QLatin1String("0");
        }
        return token >= QLatin1String("0") && token <= QLatin1String("8");
    }
} // namespace

namespace Ulxd
{
    int Message::argInt(int index, int defaultValue) const
    {
        if (index < 0 || index >= args.size()) {
            return defaultValue;
        }
        bool      ok    = false;
        const int value = args.at(index).trimmed().toInt(&ok);
        return ok ? value : defaultValue;
    }

    bool Message::argBool(int index, bool defaultValue) const
    {
        if (index < 0 || index >= args.size()) {
            return defaultValue;
        }
        const QString value = args.at(index).trimmed().toUpper();
        if (value == QLatin1String("ON") || value == QLatin1String("YES") || value == QLatin1String("TRUE")) {
            return true;
        }
        if (value == QLatin1String("OFF") || value == QLatin1String("NO") || value == QLatin1String("FALSE")) {
            return false;
        }
        return defaultValue;
    }

    QString Message::toWire() const
    {
        QString frame = QStringLiteral("< ");
        frame += verbToString(verb);

        if (channel >= 0) {
            frame += QLatin1Char(' ');
            frame += QString::number(channel);
        }

        if (!parameter.isEmpty()) {
            frame += QLatin1Char(' ');
            frame += parameter;
        }

        for (int i = 0; i < args.size(); ++i) {
            frame += QLatin1Char(' ');
            const bool quoted = i < quotedArgs.size() && quotedArgs.at(i);
            if (quoted) {
                frame += QLatin1Char('{') + args.at(i) + QLatin1Char('}');
            }
            else {
                frame += args.at(i);
            }
        }

        frame += QStringLiteral(" >");
        return frame;
    }

    Message Message::parse(const QString& frame, bool* ok)
    {
        Message message;
        message.raw = frame.trimmed();

        if (ok != nullptr) {
            *ok = false;
        }
        if (message.raw.isEmpty()) {
            return message;
        }

        // 去掉首尾尖括号
        QString body = message.raw;
        if (body.startsWith(QLatin1Char('<'))) {
            body.remove(0, 1);
        }
        if (body.endsWith(QLatin1Char('>'))) {
            body.chop(1);
        }

        const QStringList tokens = tokenize(body.trimmed());
        if (tokens.isEmpty()) {
            return message;
        }

        message.verb = verbFromString(tokens.first());
        if (message.verb == Verb::Unknown) {
            return message;
        }

        int index = 1;

        // 可选通道号：紧跟动词的数字
        if (index < tokens.size() && looksLikeChannel(tokens.at(index))) {
            message.channel = tokens.at(index).toInt();
            ++index;
        }

        if (index >= tokens.size()) {
            return message;
        }

        message.parameter = tokens.at(index).toUpper();
        ++index;

        for (; index < tokens.size(); ++index) {
            QString token = tokens.at(index);
            if (token.startsWith(QLatin1Char('{')) && token.endsWith(QLatin1Char('}'))) {
                message.args << token.mid(1, token.size() - 2);
                message.quotedArgs << true;
            }
            else {
                message.args << token;
                message.quotedArgs << false;
            }
        }

        if (ok != nullptr) {
            *ok = true;
        }
        return message;
    }
} // namespace Ulxd
