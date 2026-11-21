#pragma once

#include <QMetaType>
#include <QString>
#include <QtGlobal>

#include <cstdint>

namespace SimpleInjector {

enum class ProcessState {
    Unsupported,  ///< Not running on Windows.
    NotRunning,   ///< The process was not found.
    AccessDenied, ///< The process exists but Windows refused to open it (see errorCode).
    NoModuleBase, ///< Opened, but the main module's base address could not be read.
    Connected,    ///< Attached and ready for reads and writes.
};

struct ProcessStatus {
    ProcessState state = ProcessState::NotRunning;
    quint32 pid = 0;
    quintptr imageBase = 0;
    quint32 errorCode = 0; ///< GetLastError() when AccessDenied.
    QString version;       ///< `version-<hex>` derived from the executable's path (may be empty).

    bool operator==(const ProcessStatus &other) const
    {
        return state == other.state && pid == other.pid && imageBase == other.imageBase
            && errorCode == other.errorCode && version == other.version;
    }
    bool operator!=(const ProcessStatus &other) const { return !(*this == other); }
};

/// Source of information about the Roblox process. The real implementation uses Windows APIs;
/// tests use a fake that returns a scripted sequence of statuses.
class IProcessProbe {
public:
    virtual ~IProcessProbe() = default;

    /// Re-checks and returns the current status (may attach or detach internally).
    virtual ProcessStatus refresh() = 0;
    virtual void detach() = 0;
};

} // namespace SimpleInjector

Q_DECLARE_METATYPE(SimpleInjector::ProcessStatus)
