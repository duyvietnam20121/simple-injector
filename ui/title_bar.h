#pragma once

#include <QPoint>
#include <QWidget>

class QMainWindow;
class QMouseEvent;

namespace SimpleInjector::Ui {

/// Thanh tiêu đề tự vẽ cho cửa sổ không viền: kéo để di chuyển, nút thu nhỏ/đóng.
class TitleBar final : public QWidget {
public:
    explicit TitleBar(QMainWindow *window);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QMainWindow *window_ = nullptr;
    QPoint dragOffset_;
    bool dragging_ = false;
};

} // namespace SimpleInjector::Ui
