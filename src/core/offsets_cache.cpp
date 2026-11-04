#include "offsets_cache.h"

#include "flag_utils.h"

#include <QJsonObject>

#include <limits>

namespace SimpleInjector {

OffsetMap loadOffsetsCache(const QString &path)
{
    OffsetMap result;
    const QJsonObject cached = readJsonObject(path, {});
    for (auto it = cached.begin(); it != cached.end(); ++it) {
        bool ok = false;
        const qulonglong value = it.value().toString().toULongLong(&ok, 16);
        if (ok && value != 0 && value <= std::numeric_limits<quintptr>::max())
            result.insert(it.key(), static_cast<quintptr>(value));
    }
    return result;
}

bool saveOffsetsCache(const QString &path, const OffsetMap &offsets, QString *error)
{
    QJsonObject cache;
    for (auto it = offsets.cbegin(); it != offsets.cend(); ++it)
        cache.insert(it.key(), QString::number(static_cast<qulonglong>(it.value()), 16));
    return writeJsonAtomically(path, cache, error);
}

} // namespace SimpleInjector
