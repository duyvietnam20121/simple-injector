#pragma once

#include <QString>

namespace SimpleInjector {

/// Đường dẫn các file dữ liệu của ứng dụng.
struct StoragePaths {
    QString directory;
    QString flags;
    QString settings;
    QString offsets;
};

/// Tạo thư mục dữ liệu `<root>/SimpleInjector`, đồng thời sao chép file cũ
/// từ `<root>/ez` nếu file mới chưa tồn tại. `root` rỗng thì dùng %LOCALAPPDATA%
/// (hoặc thư mục dữ liệu cục bộ chuẩn của Qt).
StoragePaths prepareStorage(const QString &root = {});

} // namespace SimpleInjector
