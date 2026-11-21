#pragma once

#include "platform/process_probe.h"

#include <QObject>
#include <QTimer>

namespace SimpleInjector {

/// Periodically asks an IProcessProbe and emits signals when the status changes.
/// It does not own the probe (the probe must outlive the monitor), so tests can use a fake one.
class ProcessMonitor final : public QObject {
    Q_OBJECT

public:
    ProcessMonitor(IProcessProbe *probe, int intervalMs = 1500, QObject *parent = nullptr);

    /// Checks once immediately, then starts the timer.
    void start();
    void stop();
    void checkNow();

    const ProcessStatus &status() const { return status_; }
    bool isConnected() const { return status_.state == ProcessState::Connected; }

signals:
    /// Emitted when (and only when) the status differs from the previous check; the first
    /// check always emits.
    void statusChanged(const SimpleInjector::ProcessStatus &status);
    /// Emitted exactly once each time the monitor attaches to a new process
    /// (including a PID change that never passed through a disconnected state).
    void attached();

private:
    IProcessProbe *probe_ = nullptr;
    QTimer timer_;
    ProcessStatus status_;
    bool firstCheck_ = true;
};

} // namespace SimpleInjector
