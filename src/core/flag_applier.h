#pragma once

#include "offsets_cache.h"

#include <QJsonObject>
#include <QString>

#include <cstdint>

namespace SimpleInjector {

/// Cổng ghi bộ nhớ trừu tượng: bản Windows ghi vào tiến trình thật,
/// bản giả trong test chỉ ghi nhận lời gọi.
class IMemoryWriter {
public:
    virtual ~IMemoryWriter() = default;

    /// Ghi một số nguyên 32-bit tại địa chỉ tuyệt đối; true nếu ghi đủ 4 byte.
    virtual bool writeInt32(quintptr address, qint32 value) = 0;
};

/// Kết quả của một lần Apply, phân loại rõ từng nguyên nhân thất bại.
struct ApplyReport {
    int applied = 0;       ///< Ghi thành công.
    int missingOffset = 0; ///< Không có offset tương ứng.
    int invalidValue = 0;  ///< Giá trị không phải bool / int32 (ví dụ chuỗi).
    int writeFailed = 0;   ///< Ghi thất bại hoặc địa chỉ tràn số.

    int total() const { return applied + missingOffset + invalidValue + writeFailed; }
    QString summary() const;
};

/// Áp dụng từng flag: tên -> offset -> địa chỉ = imageBase + offset -> ghi int32.
ApplyReport applyFlags(const QJsonObject &flags,
                       const OffsetMap &offsets,
                       quintptr imageBase,
                       IMemoryWriter &writer);

} // namespace SimpleInjector
