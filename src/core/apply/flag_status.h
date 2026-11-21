#pragma once

#include <QString>

namespace SimpleInjector {

/// Outcome of applying one flag. The table shows only three states (pending / applied /
/// failed); the individual failure reasons are kept for reports and tests.
enum class FlagStatus {
    Pending,       ///< Not applied yet (never applied, or its value was edited since).
    Applied,       ///< Written AND read back with the expected value.
    VerifyFailed,  ///< The write reported success but the read-back differs (stale offset?).
    MissingOffset, ///< The offsets table has no entry for this flag.
    InvalidValue,  ///< The value is not a bool / 32-bit integer (for example a string).
    WriteFailed,   ///< The write failed or the target address would overflow.
};

/// True for every status that means "tried to apply and it did not work".
bool isFailed(FlagStatus status);

struct FlagResult {
    FlagStatus status = FlagStatus::Pending;
    QString detail; ///< Short English explanation, useful for logs and tests.
};

} // namespace SimpleInjector
