#pragma once

#include "core/offsets/offset_map.h"

#include <QString>

namespace SimpleInjector {

/// Extracts the `Name = 0x...` entries from an offsets header. Entries whose offset is zero or
/// does not fit in a quintptr are skipped.
///
/// Every name is stored verbatim. If a name has the form <type prefix> + <rest>, the version
/// without the prefix is stored as well (never overwriting a different verbatim name), so both
/// `FIntFoo` and `Foo` can be looked up.
OffsetMap parseOffsetsHeader(const QString &source);

} // namespace SimpleInjector
