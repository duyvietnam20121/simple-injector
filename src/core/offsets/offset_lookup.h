#pragma once

#include "core/offsets/offset_map.h"

#include <QString>

#include <optional>

namespace SimpleInjector {

/// Looks up the offset of a flag. Tries, in order:
///   1. the exact name (the offsets table normally stores names WITHOUT the type prefix, but a
///      few real names happen to start with "FFlag..."/"FInt..." and must match verbatim),
///   2. the name with one type prefix removed.
/// Returns std::nullopt if neither is present.
std::optional<quintptr> lookupOffset(const OffsetMap &offsets, const QString &flagName);

} // namespace SimpleInjector
