/**
 * @FilePath: /DOD-D_Serial/Protocol/Ulxd/UlxdTypes.cpp
 * @Description: ULX-D 协议基础类型实现
 */

#include "UlxdTypes.h"

namespace Ulxd
{
    QString verbToString(Verb verb)
    {
        switch (verb) {
            case Verb::Get: return QStringLiteral("GET");
            case Verb::Set: return QStringLiteral("SET");
            case Verb::Rep: return QStringLiteral("REP");
            case Verb::Err: return QStringLiteral("ERR");
            case Verb::Sample: return QStringLiteral("SAMPLE");
            case Verb::Unknown: return QStringLiteral("UNKNOWN");
        }
        return QStringLiteral("UNKNOWN");
    }

    Verb verbFromString(const QString& text)
    {
        const QString upper = text.trimmed().toUpper();
        if (upper == QLatin1String("GET")) {
            return Verb::Get;
        }
        if (upper == QLatin1String("SET")) {
            return Verb::Set;
        }
        if (upper == QLatin1String("REP")) {
            return Verb::Rep;
        }
        if (upper == QLatin1String("ERR")) {
            return Verb::Err;
        }
        if (upper == QLatin1String("SAMPLE")) {
            return Verb::Sample;
        }
        return Verb::Unknown;
    }

    bool isErrorValue(const QString& asciiValue)
    {
        bool      ok    = false;
        const int value = asciiValue.trimmed().toInt(&ok);
        if (!ok) {
            return false;
        }
        if (asciiValue.trimmed().size() == 3) {
            return value >= 252 && value <= 255;
        }
        if (asciiValue.trimmed().size() == 5) {
            return value >= 65532 && value <= 65535;
        }
        return false;
    }

    QString describeError(const QString& asciiValue)
    {
        if (!isErrorValue(asciiValue)) {
            return asciiValue;
        }
        return QStringLiteral("%1（设备不可用/未响应）").arg(asciiValue);
    }
} // namespace Ulxd
