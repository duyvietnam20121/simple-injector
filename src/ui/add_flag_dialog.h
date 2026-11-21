#pragma once

#include <QDialog>
#include <QJsonObject>

class QPlainTextEdit;

namespace SimpleInjector::Ui {

/// Dialog for pasting JSON or importing it from a `.json` file.
class AddFlagDialog final : public QDialog {
public:
    explicit AddFlagDialog(QWidget *parent = nullptr);

    /// Parses the text into an object; fills *error if it is invalid.
    QJsonObject flags(QString *error) const;

private:
    void importFromFile();

    static constexpr qint64 kMaxImportBytes = 5 * 1024 * 1024;
    QPlainTextEdit *editor_ = nullptr;
};

} // namespace SimpleInjector::Ui
