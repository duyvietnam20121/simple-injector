#pragma once

#include <QtGlobal>

#include <cstdint>

namespace SimpleInjector {

/// Abstract access to another process's memory. The Windows implementation touches the real
/// Roblox process; tests use a fake that records the calls.
class IProcessMemory {
public:
    virtual ~IProcessMemory() = default;

    /// Writes a 32-bit integer at an absolute address; true if all 4 bytes were written.
    virtual bool writeInt32(quintptr address, qint32 value) = 0;

    /// Reads a 32-bit integer; true if all 4 bytes were read and `*out` was filled.
    virtual bool readInt32(quintptr address, qint32 *out) = 0;
};

} // namespace SimpleInjector
