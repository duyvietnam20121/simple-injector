#include "offsets_cache.h"

#include "flag_utils.h"

#include <QJsonObject>

#include <limits>

namespace SimpleInjector {

namespace {
const QString kMetaKey = QStringLiteral("__meta");
const QString kVersionKey = QStringLiteral("version");
} // namespace

OffsetMap loadOffsetsCache(const QString &path, QString *version)
{
    OffsetMap result;
    const QJsonObject cached = readJsonObject(path, {});

    if (version) {
        *version = cached.value(kMetaKey).toObject().value(kVersionKey).toString();
    }

    for (auto it = cached.begin(); it != cached.end(); ++it) {
        if (it.key() == kMetaKey)
            continue;
        bool ok = false;
        const qulonglong value = it.value().toString().toULongLong(&ok, 16);
        if (ok && value != 0 && value <= std::numeric_limits<quintptr>::max())
            result.insert(it.key(), static_cast<quintptr>(value));
    }
    return result;
}

bool saveOffsetsCache(const QString &path, const OffsetMap &offsets,
                      const QString &version, QString *error)
{
    QJsonObject cache;
    for (auto it = offsets.cbegin(); it != offsets.cend(); ++it)
        cache.insert(it.key(), QString::number(static_cast<qulonglong>(it.value()), 16));

    if (!version.isEmpty()) {
        QJsonObject meta;
        meta.insert(kVersionKey, version);
        cache.insert(kMetaKey, meta);
    }
    return writeJsonAtomically(path, cache, error);
}

} // namespace SimpleInjector
