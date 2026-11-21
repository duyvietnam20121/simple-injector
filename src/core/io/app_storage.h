#pragma once

#include <QString>

namespace SimpleInjector {

/// Locations of the application's data files.
struct StoragePaths {
    QString directory;
    QString flags;    ///< Saved flags (ffs.json).
    QString settings; ///< User settings (settings.json).
    QString offsets;  ///< Offsets cache (offsets.json).
};

/// Creates `<root>/SimpleInjector` and, for each data file that does not exist there yet,
/// copies it from the legacy `<root>/ez` folder if present. An empty `root` means
/// %LOCALAPPDATA% (or Qt's standard local data location when that is not set).
StoragePaths prepareStorage(const QString &root = {});

} // namespace SimpleInjector
