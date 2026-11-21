#pragma once

#include "core/apply/process_memory.h"
#include "platform/process_probe.h"

namespace SimpleInjector {

/// Finds, opens, reads and writes the RobloxPlayerBeta.exe process.
///
/// Uses only documented Windows APIs (OpenProcess / ReadProcessMemory / WriteProcessMemory) with
/// the current user's own rights. On other platforms it always reports `ProcessState::Unsupported`.
class RobloxProcess final : public IProcessMemory, public IProcessProbe {
public:
    RobloxProcess() = default;
    ~RobloxProcess() override;

    RobloxProcess(const RobloxProcess &) = delete;
    RobloxProcess &operator=(const RobloxProcess &) = delete;

    /// Keeps the connection while the process is alive (a cheap handle check), otherwise
    /// scans the process list again.
    ProcessStatus refresh() override;
    void detach() override;

    bool isAttached() const { return handle_ != nullptr; }
    quintptr imageBase() const { return imageBase_; }

    bool writeInt32(quintptr address, qint32 value) override;
    bool readInt32(quintptr address, qint32 *out) override;

private:
    void *handle_ = nullptr; // HANDLE on Windows
    quint32 pid_ = 0;
    quintptr imageBase_ = 0;
    QString version_;
};

} // namespace SimpleInjector
