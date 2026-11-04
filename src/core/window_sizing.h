#pragma once

#include <QRect>
#include <QSize>

#include <algorithm>

namespace SimpleInjector {

/// Co kích thước mong muốn cho vừa vùng làm việc của màn hình (trừ thanh tác vụ),
/// chừa lại một khoảng lề: tối đa `fraction` mỗi chiều. Không bao giờ phóng to
/// quá kích thước mong muốn. Dùng cho cả kích thước ban đầu lẫn kích thước tối thiểu,
/// để cửa sổ không tràn ra ngoài màn hình nhỏ hoặc màn hình có tỉ lệ hiển thị cao.
inline QSize fitToScreen(const QSize &preferred, const QRect &available, double fraction = 0.9)
{
    const int maxWidth = std::max(1, static_cast<int>(available.width() * fraction));
    const int maxHeight = std::max(1, static_cast<int>(available.height() * fraction));
    return {std::min(preferred.width(), maxWidth), std::min(preferred.height(), maxHeight)};
}

/// Đặt `size` vào giữa vùng làm việc.
inline QRect centeredIn(const QSize &size, const QRect &available)
{
    const QPoint topLeft(available.x() + (available.width() - size.width()) / 2,
                         available.y() + (available.height() - size.height()) / 2);
    return QRect(topLeft, size);
}

} // namespace SimpleInjector
