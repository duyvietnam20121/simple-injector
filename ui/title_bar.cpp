#include "title_bar.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMouseEvent>
#include <QPushButton>

namespace SimpleInjector::Ui {

TitleBar::TitleBar(QMainWindow *window) : QWidget(window), window_(window)
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 5, 8, 5);

    auto *label = new QLabel(QStringLiteral("SIMPLE INJECTOR"), this);
    label->setStyleSheet(QStringLiteral(
        "color:#B8B8B8;font-size:10px;font-weight:bold;letter-spacing:1px;border:none;"));

    auto *minimize = new QPushButton(QStringLiteral("—"), this);
    auto *close = new QPushButton(QStringLiteral("×"), this);
    for (QPushButton *button : {minimize, close}) {
        button->setFixedSize(30, 27);
        button->setStyleSheet(QStringLiteral(
            "QPushButton{background:transparent;border:none;font-size:15px;} "
            "QPushButton:hover{color:#FF5555;}"));
    }

    layout->addWidget(label);
    layout->addStretch();
    layout->addWidget(minimize);
    layout->addWidget(close);

    connect(minimize, &QPushButton::clicked, window_, &QWidget::showMinimized);
    connect(close, &QPushButton::clicked, window_, &QWidget::close);
}

void TitleBar::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        dragOffset_ = event->globalPosition().toPoint() - window_->frameGeometry().topLeft();
        dragging_ = true;
    }
}

void TitleBar::mouseMoveEvent(QMouseEvent *event)
{
    if (dragging_ && (event->buttons() & Qt::LeftButton))
        window_->move(event->globalPosition().toPoint() - dragOffset_);
}

void TitleBar::mouseReleaseEvent(QMouseEvent *)
{
    dragging_ = false;
}

} // namespace SimpleInjector::Ui
