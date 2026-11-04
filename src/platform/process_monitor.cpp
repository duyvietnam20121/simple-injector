#include "process_monitor.h"

namespace SimpleInjector {

ProcessMonitor::ProcessMonitor(int intervalMs, QObject *parent) : QObject(parent)
{
    timer_.setInterval(intervalMs);
    connect(&timer_, &QTimer::timeout, this, &ProcessMonitor::checkNow);
}

void ProcessMonitor::start()
{
    checkNow();
    timer_.start();
}

void ProcessMonitor::checkNow()
{
    const RobloxProcess::Status next = process_.refresh();
    if (!firstCheck_ && next == status_)
        return;

    const bool wasConnected = !firstCheck_ && isConnected();
    const bool samePid = wasConnected && status_.pid == next.pid;
    firstCheck_ = false;
    status_ = next;

    emit statusChanged(status_);
    if (isConnected() && !samePid)
        emit attached();
}

} // namespace SimpleInjector
