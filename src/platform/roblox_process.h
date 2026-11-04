#pragma once

#include "core/flag_applier.h"

#include <QtGlobal>

#include <cstdint>

namespace SimpleInjector {

/// Tìm, mở và ghi vào tiến trình RobloxPlayerBeta.exe.
///
/// Chỉ dùng API Windows có tài liệu (OpenProcess / WriteProcessMemory) với quyền
/// của chính người dùng. Trên nền tảng khác, lớp này luôn báo `Unsupported`.
class RobloxProcess final : public IMemoryWriter {
public:
    enum class State {
        Unsupported,  ///< Không phải Windows.
        NotRunning,   ///< Chưa thấy tiến trình.
        AccessDenied, ///< Thấy tiến trình nhưng Windows từ chối mở (xem errorCode).
        NoModuleBase, ///< Mở được nhưng không đọc được địa chỉ module chính.
        Connected,    ///< Đã gắn, sẵn sàng ghi.
    };

    struct Status {
        State state = State::NotRunning;
        quint32 pid = 0;
        quintptr imageBase = 0;
        quint32 errorCode = 0; ///< GetLastError() khi AccessDenied.

        bool operator==(const Status &other) const
        {
            return state == other.state && pid == other.pid
                && imageBase == other.imageBase && errorCode == other.errorCode;
        }
        bool operator!=(const Status &other) const { return !(*this == other); }
    };

    RobloxProcess() = default;
    ~RobloxProcess() override;

    RobloxProcess(const RobloxProcess &) = delete;
    RobloxProcess &operator=(const RobloxProcess &) = delete;

    /// Làm mới trạng thái: giữ kết nối nếu tiến trình còn sống (kiểm tra rẻ bằng handle),
    /// ngược lại quét lại danh sách tiến trình.
    Status refresh();

    void detach();
    bool isAttached() const { return handle_ != nullptr; }
    quintptr imageBase() const { return imageBase_; }

    bool writeInt32(quintptr address, qint32 value) override;

private:
    void *handle_ = nullptr; // HANDLE trên Windows
    quint32 pid_ = 0;
    quintptr imageBase_ = 0;
};

} // namespace SimpleInjector
