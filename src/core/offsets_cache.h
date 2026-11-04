#pragma once

#include <QHash>
#include <QString>

#include <cstdint>

namespace SimpleInjector {

using OffsetMap = QHash<QString, quintptr>;

/// Đọc cache offsets (JSON: tên -> chuỗi hex). Mục hỏng hoặc bằng 0 bị bỏ qua.
/// Nếu `version` khác null, nhận phiên bản Roblox lưu kèm (rỗng nếu cache cũ không có).
OffsetMap loadOffsetsCache(const QString &path, QString *version = nullptr);

/// Ghi cache nguyên tử. Phiên bản (nếu có) nằm trong khóa riêng `__meta`,
/// nên file vẫn đọc được bởi bản cũ của ứng dụng.
bool saveOffsetsCache(const QString &path, const OffsetMap &offsets,
                      const QString &version = {}, QString *error = nullptr);

} // namespace SimpleInjector
