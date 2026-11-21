#include "core/apply/flag_status.h"

namespace SimpleInjector {

bool isFailed(FlagStatus status)
{
    switch (status) {
    case FlagStatus::VerifyFailed:
    case FlagStatus::MissingOffset:
    case FlagStatus::InvalidValue:
    case FlagStatus::WriteFailed:
        return true;
    case FlagStatus::Pending:
    case FlagStatus::Applied:
        return false;
    }
    return false;
}

} // namespace SimpleInjector
