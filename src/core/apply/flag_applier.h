#pragma once

#include "core/apply/flag_status.h"
#include "core/apply/process_memory.h"
#include "core/offsets/offset_map.h"

#include <QHash>
#include <QJsonObject>
#include <QString>

namespace SimpleInjector {

/// Result of one Apply run: a per-flag outcome plus convenience counters.
struct ApplyReport {
    /// Per-flag results, keyed by the flag name exactly as it is in the store.
    QHash<QString, FlagResult> results;

    int count(FlagStatus status) const;
    int total() const { return static_cast<int>(results.size()); }
    int applied() const { return count(FlagStatus::Applied); }
    int failed() const { return total() - applied(); }

    /// One-line summary, e.g. "Applied 2/4 flags · 2 failed".
    QString summary() const;
};

/// Applies each flag: name -> offset -> address = imageBase + offset -> write int32, then read it
/// back to confirm. A read-back mismatch usually means the offset is stale, or the game
/// overwrote the value right after the write.
ApplyReport applyFlags(const QJsonObject &flags,
                       const OffsetMap &offsets,
                       quintptr imageBase,
                       IProcessMemory &memory);

} // namespace SimpleInjector
