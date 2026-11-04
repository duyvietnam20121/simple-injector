#pragma once

#include "process_probe.h"

#include <QObject>
#include <QTimer>

namespace SimpleInjector {

/// Định kỳ hỏi IProcessProbe và phát tín hiệu khi trạng thái thay đổi.
/// Không sở hữu probe (probe phải sống lâu hơn monitor), nên test dùng được probe giả.
class ProcessMonitor final : public QObject {
    Q_OBJECT

public:
    ProcessMonitor(IProcessProbe *probe, int intervalMs = 1500, QObject *parent = nullptr);

    /// Kiểm tra ngay một lần rồi bắt đầu hẹn giờ.
    void start();
    void stop();
    void checkNow();

    const ProcessStatus &status() const { return status_; }
    bool isConnected() const { return status_.state == ProcessState::Connected; }

signals:
    /// Phát khi (và chỉ khi) trạng thái khác lần kiểm tra trước; lần kiểm tra đầu luôn phát.
    void statusChanged(const SimpleInjector::ProcessStatus &status);
    /// Phát đúng một lần mỗi khi vừa gắn vào một tiến trình mới
    /// (kể cả khi đổi PID mà không qua trạng thái ngắt kết nối).
    void attached();

private:
    IProcessProbe *probe_ = nullptr;
    QTimer timer_;
    ProcessStatus status_;
    bool firstCheck_ = true;
};

} // namespace SimpleInjector
