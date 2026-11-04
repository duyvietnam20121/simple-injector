#include "flag_applier.h"

#include "flag_utils.h"

#include <limits>

namespace SimpleInjector {

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

    for (auto it = flags.constBegin(); it != flags.constEnd(); ++it) {
        const auto offsetIt = offsets.constFind(cleanPrefix(it.key()));
        if (offsetIt == offsets.cend()) {
            ++report.missingOffset;
            continue;
        }

        qint32 value = 0;
        if (!parseInt32Value(jsonValueToString(it.value()), &value)) {
            ++report.invalidValue;
            continue;
        }

        const quintptr offset = offsetIt.value();
        if (offset > std::numeric_limits<quintptr>::max() - imageBase) {
            ++report.writeFailed; // imageBase + offset sẽ tràn số
            continue;
        }
        const quintptr address = imageBase + offset;

        if (!writer.writeInt32(address, value)) {
            ++report.writeFailed;
            continue;
        }

        qint32 readBack = 0;
        if (writer.readInt32(address, &readBack) && readBack == value)
            ++report.applied;
        else
            ++report.verifyFailed;
    }
    return report;
}

} // namespace SimpleInjector
