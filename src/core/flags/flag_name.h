#pragma once

#include <QString>

namespace SimpleInjector {

/// Removes ONE Roblox flag-type prefix (FFlag, DFInt, FString, SFFlag, ...) and surrounding
/// whitespace.
///
/// No rule is applied to the character after the prefix: many real flags have names that
/// start with a lowercase letter (for example `FIntkCageHoleNum`), so they must be stripped
/// too. Names that merely look like they carry a prefix are handled at lookup time
/// (see lookupOffset), not here.
QString cleanPrefix(QString name);

} // namespace SimpleInjector
