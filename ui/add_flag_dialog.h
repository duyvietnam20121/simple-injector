#pragma once

#include <QDialog>
#include <QJsonObject>

class QPlainTextEdit;

namespace SimpleInjector::Ui {

/// Hộp thoại dán JSON hoặc nhập từ file `.json`.
class AddFlagDialog final : public QDialog {
public:
    explicit AddFlagDialog(QWidget *parent = nullptr);

    /// Phân tích nội dung thành object; điền *error nếu sai.
    QJsonObject flags(QString *error) const;

private:
    void importFromFile();

    static constexpr qint64 kMaxImportBytes = 5 * 1024 * 1024;
    QPlainTextEdit *editor_ = nullptr;
};

} // namespace SimpleInjector::Ui
