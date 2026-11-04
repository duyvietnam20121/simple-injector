#include "flag_applier.h"

#include "flag_utils.h"

#include <limits>

namespace SimpleInjector {

QString ApplyReport::summary() const
{
    return QStringLiteral("Đã xong · %1 đã ghi · %2 thiếu offset · %3 giá trị không hỗ trợ · %4 lỗi ghi")
        .arg(applied)
        .arg(missingOffset)
        .arg(invalidValue)
        .arg(writeFailed);
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

        if (writer.writeInt32(imageBase + offset, value))
            ++report.applied;
        else
            ++report.writeFailed;
    }
    return report;
}

} // namespace SimpleInjector
