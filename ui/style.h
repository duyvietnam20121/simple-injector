#pragma once

#include <QString>

namespace SimpleInjector::Ui {

/// Stylesheet tối dùng chung cho cửa sổ chính và hộp thoại.
inline QString appStyleSheet()
{
    return QStringLiteral(R"(
QWidget#MainContainer {
    background-color: #0F0F11;
    border: 1px solid #28282B;
    border-radius: 12px;
}
QLabel { color: #E0E0E0; font-family: 'Segoe UI'; }
QLineEdit, QPlainTextEdit {
    background-color: #0A0A0A;
    border: 1px solid #2A2A2C;
    border-radius: 5px;
    color: #E0E0E0;
    padding: 7px;
    selection-background-color: #5A1717;
}
QPushButton {
    background-color: #1E1E21;
    border: 1px solid #303034;
    border-radius: 5px;
    color: #E0E0E0;
    padding: 6px 10px;
    font-weight: 500;
}
QPushButton:hover { background-color: #302022; border-color: #713333; }
QPushButton:disabled { color: #555; background-color: #151517; border-color: #222; }
QPushButton#ApplyBtn { background-color: #351010; border-color: #6B2424; font-size: 12px; font-weight: bold; }
QPushButton#ApplyBtn:hover { background-color: #501717; }
QTableWidget { background-color: #080809; border: none; gridline-color: #1A1A1D; outline: none; color: #C9C9C9; selection-background-color: #351919; }
QTableWidget::item { padding: 5px; }
QHeaderView::section { background-color: #111113; color: #777; padding: 7px; border: none; font-weight: bold; font-size: 10px; }
QScrollBar:vertical { border: none; background: #0F0F11; width: 7px; }
QScrollBar::handle:vertical { background: #343438; min-height: 22px; border-radius: 3px; }
QCheckBox { color: #AAA; font-size: 11px; spacing: 7px; }
)");
}

inline QString statusStyle(bool good)
{
    return good
        ? QStringLiteral("color:#61D786;font-weight:bold;font-size:11px;border:none;")
        : QStringLiteral("color:#FF6666;font-weight:bold;font-size:11px;border:none;");
}

} // namespace SimpleInjector::Ui
