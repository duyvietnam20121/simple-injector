#include "core/offsets/offsets_header_parser.h"

#include "core/flags/flag_name.h"

#include <QPair>
#include <QRegularExpression>
#include <QVector>

#include <limits>

namespace SimpleInjector {

OffsetMap parseOffsetsHeader(const QString &source)
{
    static const QRegularExpression pattern(
        QStringLiteral(R"((\w+)\s*=\s*(0x[0-9A-Fa-f]+))"));

    OffsetMap parsed;
    QVector<QPair<QString, quintptr>> inOrder;

    auto iterator = pattern.globalMatch(source);
    while (iterator.hasNext()) {
        const QRegularExpressionMatch match = iterator.next();
        bool ok = false;
        const qulonglong value = match.captured(2).mid(2).toULongLong(&ok, 16);
        if (!ok || value == 0 || value > std::numeric_limits<quintptr>::max())
            continue;

        const QString name = match.captured(1);
        parsed.insert(name, static_cast<quintptr>(value)); // verbatim names always win
        inOrder.append({name, static_cast<quintptr>(value)});
    }

    // Prefix-less variants: only added when no verbatim name already uses that spelling.
    for (const auto &entry : std::as_const(inOrder)) {
        const QString cleaned = cleanPrefix(entry.first);
        if (!cleaned.isEmpty() && cleaned != entry.first && !parsed.contains(cleaned))
            parsed.insert(cleaned, entry.second);
    }
    return parsed;
}

} // namespace SimpleInjector
