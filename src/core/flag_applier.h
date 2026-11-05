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

/// Trạng thái của TỪNG flag, để người dùng biết chính xác flag nào áp dụng được và vì sao không.
enum class FlagStatus {
    Unknown,       ///< Chưa biết (chưa có bảng offsets để đối chiếu).
    Ready,         ///< Có offset và giá trị hợp lệ, chưa Apply.
    Applied,       ///< Đã ghi VÀ đọc lại thấy đúng giá trị.
    VerifyFailed,  ///< Ghi báo thành công nhưng đọc lại không khớp (offset cũ / bị ghi đè).
    MissingOffset, ///< Bảng offsets không có flag này.
    InvalidValue,  ///< Giá trị không phải bool / int32 (ví dụ chuỗi), không thể Apply.
    WriteFailed,   ///< Ghi thất bại hoặc địa chỉ tràn số.
};

/// Flag nào đáng chú ý (không áp dụng được hoặc áp dụng không xác nhận được).
bool isProblem(FlagStatus status);

struct FlagResult {
    FlagStatus status = FlagStatus::Unknown;
    QString detail; ///< Giải thích ngắn gọn bằng tiếng Việt, dùng làm tooltip.
};

/// Dự đoán trạng thái TRƯỚC khi Apply, chỉ dựa vào tên, giá trị và bảng offsets
/// (không chạm bộ nhớ): Unknown / Ready / MissingOffset / InvalidValue.
FlagResult previewFlag(const QString &name, const QString &value, const OffsetMap &offsets);

/// Kết quả của một lần Apply, phân loại rõ từng nguyên nhân thất bại.
struct ApplyReport {
    int applied = 0;        ///< Ghi thành công VÀ đọc lại thấy đúng giá trị.
    int verifyFailed = 0;   ///< Ghi báo thành công nhưng đọc lại không khớp (hoặc không đọc được).
    int missingOffset = 0;  ///< Không có offset tương ứng.
    int invalidValue = 0;   ///< Giá trị không phải bool / int32 (ví dụ chuỗi).
    int writeFailed = 0;    ///< Ghi thất bại hoặc địa chỉ tràn số.

    /// Kết quả chi tiết theo từng flag (khóa là tên flag đúng như trong kho).
    QHash<QString, FlagResult> results;

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
