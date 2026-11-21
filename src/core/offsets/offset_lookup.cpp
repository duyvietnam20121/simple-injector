#include "core/offsets/offset_lookup.h"

#include "core/flags/flag_name.h"

namespace SimpleInjector {

std::optional<quintptr> lookupOffset(const OffsetMap &offsets, const QString &flagName)
{
    const QString trimmed = flagName.trimmed();

    const auto exact = offsets.constFind(trimmed);
    if (exact != offsets.cend())
        return exact.value();

    const QString cleaned = cleanPrefix(trimmed);
    if (cleaned != trimmed) {
        const auto stripped = offsets.constFind(cleaned);
        if (stripped != offsets.cend())
            return stripped.value();
    }
    return std::nullopt;
}

} // namespace SimpleInjector
