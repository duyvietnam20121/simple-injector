#pragma once

#include "core/flag_applier.h"
#include "process_probe.h"

namespace SimpleInjector {

/// Tìm, mở, đọc và ghi vào tiến trình RobloxPlayerBeta.exe.
///
/// Chỉ dùng API Windows có tài liệu (OpenProcess / ReadProcessMemory /
/// WriteProcessMemory) với quyền của chính người dùng. Trên nền tảng khác,
/// lớp này luôn báo `ProcessState::Unsupported`.
class RobloxProcess final : public IMemoryWriter, public IProcessProbe {
public:
    RobloxProcess() = default;
    ~RobloxProcess() override;

    RobloxProcess(const RobloxProcess &) = delete;
    RobloxProcess &operator=(const RobloxProcess &) = delete;

    /// Giữ kết nối nếu tiến trình còn sống (kiểm tra rẻ bằng handle),
    /// ngược lại quét lại danh sách tiến trình.
    ProcessStatus refresh() override;
    void detach() override;

    bool isAttached() const { return handle_ != nullptr; }
    quintptr imageBase() const { return imageBase_; }

    bool writeInt32(quintptr address, qint32 value) override;
    bool readInt32(quintptr address, qint32 *out) override;

private:
    void *handle_ = nullptr; // HANDLE trên Windows
    quint32 pid_ = 0;
    quintptr imageBase_ = 0;
    QString version_;
};

} // namespace SimpleInjector
