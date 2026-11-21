#include "core/apply/flag_applier.h"

#include "core/flags/flag_name.h"
#include "core/flags/flag_value.h"
#include "core/offsets/offset_lookup.h"

#include <limits>
#include <optional>

namespace SimpleInjector {

namespace {

QString hex(quintptr value)
{
    return QStringLiteral("0x") + QString::number(static_cast<qulonglong>(value), 16).toUpper();
}

FlagResult missingOffsetResult(const QString &name, const OffsetMap &offsets)
{
    if (offsets.isEmpty())
        return {FlagStatus::MissingOffset, QStringLiteral("The offsets table is empty.")};

    const QString trimmed = name.trimmed();
    const QString cleaned = cleanPrefix(trimmed);
    const QString tried = (cleaned == trimmed)
        ? QStringLiteral("\"%1\"").arg(trimmed)
        : QStringLiteral("\"%1\" or \"%2\"").arg(trimmed, cleaned);
    return {FlagStatus::MissingOffset,
            QStringLiteral("The offsets table has no entry for %1 (renamed, removed, or a different "
                           "Roblox version).").arg(tried)};
}

} // namespace

int ApplyReport::count(FlagStatus status) const
{
    int matching = 0;
    for (const FlagResult &result : results) {
        if (result.status == status)
            ++matching;
    }
    return matching;
}

QString ApplyReport::summary() const
{
    return failed() == 0
        ? QStringLiteral("Applied %1/%1 flags").arg(applied())
        : QStringLiteral("Applied %1/%2 flags · %3 failed").arg(applied()).arg(total()).arg(failed());
}

ApplyReport applyFlags(const QJsonObject &flags,
                       const OffsetMap &offsets,
                       quintptr imageBase,
                       IProcessMemory &memory)
{
    ApplyReport report;

    for (auto it = flags.constBegin(); it != flags.constEnd(); ++it) {
        const QString &key = it.key();
        FlagResult &result = report.results[key];

        const std::optional<quintptr> offset = lookupOffset(offsets, key);
        if (!offset) {
            result = missingOffsetResult(key, offsets);
            continue;
        }

        const QString valueText = jsonValueToString(it.value());
        qint32 expected = 0;
        if (!parseInt32Value(valueText, &expected)) {
            result = {FlagStatus::InvalidValue,
                      QStringLiteral("The value \"%1\" is not true/false or a 32-bit integer, so it "
                                     "cannot be applied.").arg(valueText)};
            continue;
        }

        if (*offset > std::numeric_limits<quintptr>::max() - imageBase) {
            result = {FlagStatus::WriteFailed,
                      QStringLiteral("imageBase + %1 overflows; the offset is not valid.").arg(hex(*offset))};
            continue;
        }
        const quintptr address = imageBase + *offset;

        if (!memory.writeInt32(address, expected)) {
            result = {FlagStatus::WriteFailed,
                      QStringLiteral("Writing to %1 failed (the memory is not writable or Roblox "
                                     "has exited).").arg(hex(address))};
            continue;
        }

        qint32 readBack = 0;
        if (!memory.readInt32(address, &readBack)) {
            result = {FlagStatus::VerifyFailed,
                      QStringLiteral("Wrote %1 but could not read it back to confirm.").arg(hex(address))};
        } else if (readBack != expected) {
            result = {FlagStatus::VerifyFailed,
                      QStringLiteral("Wrote %1 to %2 but read back %3: the offset may be stale or "
                                     "the game overwrote the value.")
                          .arg(expected).arg(hex(address)).arg(readBack)};
        } else {
            result = {FlagStatus::Applied,
                      QStringLiteral("Wrote %1 at %2 and read it back correctly.")
                          .arg(expected).arg(hex(address))};
        }
    }
    return report;
}

} // namespace SimpleInjector
