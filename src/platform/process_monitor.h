#pragma once

#include "roblox_process.h"

#include <QObject>
#include <QTimer>

namespace SimpleInjector {

/// Định kỳ kiểm tra Roblox và phát tín hiệu khi trạng thái thay đổi.
/// Lớp này sở hữu RobloxProcess nên giao diện không phải chạm vào API Windows.
class ProcessMonitor final : public QObject {
    Q_OBJECT

public:
    explicit ProcessMonitor(int intervalMs = 1500, QObject *parent = nullptr);

    void start();
    void checkNow();

    RobloxProcess &process() { return process_; }
    const RobloxProcess::Status &status() const { return status_; }
    bool isConnected() const { return status_.state == RobloxProcess::State::Connected; }

signals:
    /// Phát khi (và chỉ khi) trạng thái khác lần kiểm tra trước.
    void statusChanged(const SimpleInjector::RobloxProcess::Status &status);
    /// Phát đúng một lần mỗi khi vừa gắn vào một tiến trình Roblox mới.
    void attached();

private:
    RobloxProcess process_;
    QTimer timer_;
    RobloxProcess::Status status_;
    bool firstCheck_ = true;
};

} // namespace SimpleInjector
