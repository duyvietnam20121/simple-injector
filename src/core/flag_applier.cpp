#include "flag_applier.h"

#include "flag_utils.h"

#include <limits>
#include <optional>

namespace SimpleInjector {

namespace {

QString hex(quintptr value)
{
    return QStringLiteral("0x") + QString::number(static_cast<qulonglong>(value), 16).toUpper();
}

} // namespace

bool isProblem(FlagStatus status)
{
    switch (status) {
    case FlagStatus::VerifyFailed:
    case FlagStatus::MissingOffset:
    case FlagStatus::InvalidValue:
    case FlagStatus::WriteFailed:
        return true;
    case FlagStatus::Unknown:
    case FlagStatus::Ready:
    case FlagStatus::Applied:
        return false;
    }
    return false;
}

FlagResult previewFlag(const QString &name, const QString &value, const OffsetMap &offsets)
{
    if (offsets.isEmpty())
        return {FlagStatus::Unknown, QStringLiteral("Chưa có bảng offsets để đối chiếu.")};

    const std::optional<quintptr> offset = lookupOffset(offsets, name);
    if (!offset) {
        const QString cleaned = cleanPrefix(name);
        const QString tried = (cleaned == name.trimmed())
            ? QStringLiteral("\"%1\"").arg(name.trimmed())
            : QStringLiteral("\"%1\" và \"%2\"").arg(name.trimmed(), cleaned);
        return {FlagStatus::MissingOffset,
                QStringLiteral("Bảng offsets không có %1 (flag đổi tên, đã bị xóa, đổi phiên bản hoặc gõ sai).")
                    .arg(tried)};
    }

    qint32 ignored = 0;
    if (!parseInt32Value(value, &ignored)) {
        return {FlagStatus::InvalidValue,
                QStringLiteral("Giá trị \"%1\" không phải true/false hoặc số nguyên 32-bit nên không thể Apply.")
                    .arg(value)};
    }

    return {FlagStatus::Ready,
            QStringLiteral("Có offset %1, giá trị hợp lệ. Chưa Apply.").arg(hex(*offset))};
}

QString ApplyReport::summary() const
{
    QString text = QStringLiteral("Đã xong · %1 ghi và xác nhận").arg(applied);
    if (verifyFailed > 0)
        text += QStringLiteral(" · %1 ghi nhưng đọc lại không khớp (offset có thể đã cũ)").arg(verifyFailed);
    text += QStringLiteral(" · %1 thiếu offset · %2 giá trị không hỗ trợ · %3 lỗi ghi")
                .arg(missingOffset)
                .arg(invalidValue)
                .arg(writeFailed);
    return text;
}

ApplyReport applyFlags(const QJsonObject &flags,
                       const OffsetMap &offsets,
                       quintptr imageBase,
                       IMemoryWriter &writer)
{
    ApplyReport report;

    auto record = [&report](const QString &key, FlagStatus status, int ApplyReport::*counter,
                            QString detail) {
        ++(report.*counter);
        report.results.insert(key, {status, std::move(detail)});
    };

    for (auto it = flags.constBegin(); it != flags.constEnd(); ++it) {
        const QString key = it.key();
        const QString value = jsonValueToString(it.value());

        // Hai lỗi "không cần chạm bộ nhớ" dùng chung logic với phần dự đoán.
        const FlagResult preview = previewFlag(key, value, offsets);
        if (preview.status == FlagStatus::MissingOffset || preview.status == FlagStatus::Unknown) {
            record(key, FlagStatus::MissingOffset, &ApplyReport::missingOffset,
                   preview.status == FlagStatus::Unknown
                       ? QStringLiteral("Bảng offsets đang rỗng.")
                       : preview.detail);
            continue;
        }
        if (preview.status == FlagStatus::InvalidValue) {
            record(key, FlagStatus::InvalidValue, &ApplyReport::invalidValue, preview.detail);
            continue;
        }

        qint32 expected = 0;
        parseInt32Value(value, &expected); // đã được previewFlag xác nhận hợp lệ

        const quintptr offset = lookupOffset(offsets, key).value_or(0); // previewFlag đã đảm bảo có
        if (offset > std::numeric_limits<quintptr>::max() - imageBase) {
            record(key, FlagStatus::WriteFailed, &ApplyReport::writeFailed,
                   QStringLiteral("Địa chỉ imageBase + %1 bị tràn số, offset không hợp lệ.").arg(hex(offset)));
            continue;
        }
        const quintptr address = imageBase + offset;

        if (!writer.writeInt32(address, expected)) {
            record(key, FlagStatus::WriteFailed, &ApplyReport::writeFailed,
                   QStringLiteral("Ghi vào %1 thất bại (vùng nhớ không cho ghi hoặc Roblox đã thoát).")
                       .arg(hex(address)));
            continue;
        }

        qint32 readBack = 0;
        if (!writer.readInt32(address, &readBack)) {
            record(key, FlagStatus::VerifyFailed, &ApplyReport::verifyFailed,
                   QStringLiteral("Đã ghi %1 nhưng không đọc lại được để xác nhận.").arg(hex(address)));
        } else if (readBack != expected) {
            record(key, FlagStatus::VerifyFailed, &ApplyReport::verifyFailed,
                   QStringLiteral("Ghi %1 vào %2 nhưng đọc lại ra %3: offset có thể đã cũ hoặc game ghi đè.")
                       .arg(expected).arg(hex(address)).arg(readBack));
        } else {
            record(key, FlagStatus::Applied, &ApplyReport::applied,
                   QStringLiteral("Đã ghi %1 tại %2 và đọc lại đúng.").arg(expected).arg(hex(address)));
        }
    }
    return report;
}

} // namespace SimpleInjector
