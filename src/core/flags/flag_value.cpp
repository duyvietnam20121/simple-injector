#include "core/flags/flag_value.h"

#include <limits>

namespace SimpleInjector {

QString jsonValueToString(const QJsonValue &value, bool *ok)
{
    if (ok)
        *ok = true;

    if (value.isString())
        return value.toString();
    if (value.isBool())
        return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    if (value.isDouble())
        return QString::number(value.toDouble(), 'g', 16);

    if (ok)
        *ok = false;
    return {};
}

bool parseInt32Value(const QString &text, qint32 *out)
{
    if (!out)
        return false;

    const QString normalized = text.trimmed().toLower();
    if (normalized == QStringLiteral("true")) {
        *out = 1;
        return true;
    }
    if (normalized == QStringLiteral("false")) {
        *out = 0;
        return true;
    }

    bool ok = false;
    const qlonglong parsed = normalized.startsWith(QStringLiteral("0x"))
        ? normalized.mid(2).toLongLong(&ok, 16)
        : normalized.toLongLong(&ok, 10);

    if (!ok || parsed < std::numeric_limits<qint32>::min()
        || parsed > std::numeric_limits<qint32>::max()) {
        return false;
    }

    *out = static_cast<qint32>(parsed);
    return true;
}

} // namespace SimpleInjector
