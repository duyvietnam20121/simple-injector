#pragma once

#include <QPoint>
#include <QRect>
#include <QSize>

#include <algorithm>

namespace SimpleInjector {

/// Shrinks a preferred window size so it fits the screen's available area (taskbar excluded),
/// leaving a margin: at most `fraction` of each dimension. Never enlarges beyond the preferred
/// size. Used for both the initial and the minimum size so the window cannot overflow small or
/// highly scaled screens.
inline QSize fitToScreen(const QSize &preferred, const QRect &available, double fraction = 0.9)
{
    const int maxWidth = std::max(1, static_cast<int>(available.width() * fraction));
    const int maxHeight = std::max(1, static_cast<int>(available.height() * fraction));
    return {std::min(preferred.width(), maxWidth), std::min(preferred.height(), maxHeight)};
}

/// Places `size` in the middle of the available area.
inline QRect centeredIn(const QSize &size, const QRect &available)
{
    const QPoint topLeft(available.x() + (available.width() - size.width()) / 2,
                         available.y() + (available.height() - size.height()) / 2);
    return QRect(topLeft, size);
}

} // namespace SimpleInjector
