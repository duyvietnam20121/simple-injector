#pragma once

#include <QMetaType>
#include <QString>
#include <QtGlobal>

#include <cstdint>

namespace SimpleInjector {

enum class ProcessState {
    Unsupported,  ///< Không phải Windows.
    NotRunning,   ///< Chưa thấy tiến trình.
    AccessDenied, ///< Thấy tiến trình nhưng Windows từ chối mở (xem errorCode).
    NoModuleBase, ///< Mở được nhưng không đọc được địa chỉ module chính.
    Connected,    ///< Đã gắn, sẵn sàng đọc/ghi.
};

struct ProcessStatus {
    ProcessState state = ProcessState::NotRunning;
    quint32 pid = 0;
    quintptr imageBase = 0;
    quint32 errorCode = 0; ///< GetLastError() khi AccessDenied.
    QString version;       ///< `version-<hex>` suy ra từ đường dẫn file chạy (có thể rỗng).

    bool operator==(const ProcessStatus &other) const
    {
        return state == other.state && pid == other.pid && imageBase == other.imageBase
            && errorCode == other.errorCode && version == other.version;
    }
    bool operator!=(const ProcessStatus &other) const { return !(*this == other); }
};

/// Nguồn thông tin về tiến trình Roblox. Bản thật dùng API Windows;
/// bản giả trong test trả về chuỗi trạng thái được kịch bản hóa trước.
class IProcessProbe {
public:
    virtual ~IProcessProbe() = default;

    /// Kiểm tra lại và trả về trạng thái hiện tại (có thể gắn/tháo kết nối bên trong).
    virtual ProcessStatus refresh() = 0;
    virtual void detach() = 0;
};

} // namespace SimpleInjector

Q_DECLARE_METATYPE(SimpleInjector::ProcessStatus)
