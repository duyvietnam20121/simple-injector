#pragma once

#include <QString>

namespace SimpleInjector {

/// Kết quả so khớp phiên bản client Roblox với phiên bản mà bảng offsets được tạo ra.
enum class VersionMatch {
    Unknown,  ///< Thiếu thông tin ở một trong hai phía, không thể kết luận.
    Match,    ///< Cùng phiên bản.
    Mismatch, ///< Khác phiên bản: offsets có thể đã cũ.
};

/// Lấy chuỗi `version-<hex>` từ header offsets. Ưu tiên khai báo
/// `ClientVersion = "version-..."`, nếu không có thì dùng chuỗi đầu tiên xuất hiện
/// trong 2 KB đầu (dòng chú thích do trình dump sinh ra). Trả về chuỗi rỗng nếu không thấy.
QString parseOffsetsVersion(const QString &source);

/// Suy ra phiên bản từ đường dẫn file chạy, ví dụ
/// `C:\...\Roblox\Versions\version-abc123\RobloxPlayerBeta.exe` -> `version-abc123`.
/// Chấp nhận cả dấu `\` lẫn `/`. Trả về chuỗi rỗng nếu thư mục cha không có dạng đó.
QString versionFromImagePath(const QString &imagePath);

/// So sánh không phân biệt hoa/thường; chuỗi rỗng cho kết quả Unknown.
VersionMatch compareVersions(const QString &offsetsVersion, const QString &clientVersion);

} // namespace SimpleInjector
