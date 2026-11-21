#pragma once

#include <QString>

namespace SimpleInjector {

/// Result of comparing the running Roblox client's version with the version the offsets
/// table was generated for.
enum class VersionMatch {
    Unknown,  ///< One of the two versions is missing; no conclusion possible.
    Match,    ///< Same version.
    Mismatch, ///< Different versions: the offsets may be outdated.
};

/// Extracts the `version-<hex>` string from an offsets header. Prefers a declaration like
/// `ClientVersion = "version-..."`; otherwise falls back to the first such string in the first
/// 2 KB (the comment line written by the dumper). Returns an empty string if none is found.
QString parseOffsetsVersion(const QString &source);

/// Derives the version from an executable path, e.g.
/// `C:\...\Roblox\Versions\version-abc123\RobloxPlayerBeta.exe` -> `version-abc123`.
/// Accepts both `\` and `/`. Returns an empty string if the parent folder has no such name.
QString versionFromImagePath(const QString &imagePath);

/// Case-insensitive comparison; an empty string on either side gives Unknown.
VersionMatch compareVersions(const QString &offsetsVersion, const QString &clientVersion);

} // namespace SimpleInjector
