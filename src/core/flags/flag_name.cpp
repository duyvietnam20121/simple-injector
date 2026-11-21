#include "core/flags/flag_name.h"

#include <QStringList>

namespace SimpleInjector {

QString cleanPrefix(QString name)
{
    name = name.trimmed();

    // A longer prefix must come before any prefix it contains
    // (DFString before FString, DFFlag before FFlag, ...).
    static const QStringList prefixes = {
        QStringLiteral("DFString"), QStringLiteral("FString"),
        QStringLiteral("DFInt"),    QStringLiteral("FInt"),
        QStringLiteral("DFFlag"),   QStringLiteral("SFFlag"),
        QStringLiteral("FFlag"),    QStringLiteral("DFLog"),
        QStringLiteral("FLog"),
    };

    for (const QString &prefix : prefixes) {
        if (!name.startsWith(prefix, Qt::CaseSensitive))
            continue;
        name.remove(0, prefix.size());
        break;
    }
    return name.trimmed();
}

} // namespace SimpleInjector
