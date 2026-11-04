#pragma once

#include "offsets_cache.h"

#include <QJsonObject>
#include <QString>

#include <cstdint>

namespace SimpleInjector {

/// Cổng truy cập bộ nhớ trừu tượng: bản Windows chạm vào tiến trình thật,
/// bản giả trong test chỉ ghi nhận lời gọi.
class IMemoryWriter {
public:
    virtual ~IMemoryWriter() = default;

    /// Ghi một số nguyên 32-bit tại địa chỉ tuyệt đối; true nếu ghi đủ 4 byte.
    virtual bool writeInt32(quintptr address, qint32 value) = 0;

    /// Đọc một số nguyên 32-bit; true nếu đọc đủ 4 byte và đã điền *out.
    virtual bool readInt32(quintptr address, qint32 *out) = 0;
};

/// Kết quả của một lần Apply, phân loại rõ từng nguyên nhân thất bại.
struct ApplyReport {
    int applied = 0;        ///< Ghi thành công VÀ đọc lại thấy đúng giá trị.
    int verifyFailed = 0;   ///< Ghi báo thành công nhưng đọc lại không khớp (hoặc không đọc được).
    int missingOffset = 0;  ///< Không có offset tương ứng.
    int invalidValue = 0;   ///< Giá trị không phải bool / int32 (ví dụ chuỗi).
    int writeFailed = 0;    ///< Ghi thất bại hoặc địa chỉ tràn số.

    int total() const { return applied + verifyFailed + missingOffset + invalidValue + writeFailed; }
    QString summary() const;
};

/// Áp dụng từng flag: tên -> offset -> địa chỉ = imageBase + offset -> ghi int32,
/// rồi đọc lại để xác nhận. Đọc lại không khớp thường nghĩa là offset đã cũ,
/// hoặc game đã tự ghi đè giá trị ngay sau đó.
ApplyReport applyFlags(const QJsonObject &flags,
                       const OffsetMap &offsets,
                       quintptr imageBase,
                       IMemoryWriter &writer);

} // namespace SimpleInjector
