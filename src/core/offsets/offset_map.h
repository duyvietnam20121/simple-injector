#pragma once

#include <QHash>
#include <QString>

#include <cstdint>

namespace SimpleInjector {

/// Flag name -> offset of its value, relative to the base address of Roblox's main module.
using OffsetMap = QHash<QString, quintptr>;

} // namespace SimpleInjector
