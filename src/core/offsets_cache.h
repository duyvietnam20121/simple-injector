#pragma once

#include <QHash>
#include <QString>

#include <cstdint>

namespace SimpleInjector {

using OffsetMap = QHash<QString, quintptr>;

/// Đọc cache offsets (JSON: tên -> chuỗi hex). Mục hỏng hoặc bằng 0 bị bỏ qua.
OffsetMap loadOffsetsCache(const QString &path);

/// Ghi cache offsets nguyên tử.
bool saveOffsetsCache(const QString &path, const OffsetMap &offsets, QString *error = nullptr);

} // namespace SimpleInjector
