#pragma once

#include "core/offsets/offset_map.h"

#include <QString>

namespace SimpleInjector {

/// Reads the offsets cache (JSON: name -> hex string). Corrupt or zero entries are skipped.
/// If `version` is not null it receives the Roblox version stored with the cache
/// (empty if an older cache has none).
OffsetMap loadOffsetsCache(const QString &path, QString *version = nullptr);

/// Writes the cache atomically. The version (if any) lives under the separate `__meta` key,
/// so older builds of the app can still read the file.
bool saveOffsetsCache(const QString &path, const OffsetMap &offsets,
                      const QString &version = {}, QString *error = nullptr);

} // namespace SimpleInjector
