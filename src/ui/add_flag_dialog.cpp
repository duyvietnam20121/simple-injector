#include "ui/add_flag_dialog.h"

#include "core/io/json_io.h"
#include "ui/style.h"

#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace SimpleInjector::Ui {

AddFlagDialog::AddFlagDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Add FFlags"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setMinimumSize(500, 420);
    setStyleSheet(appStyleSheet());

    auto *layout = new QVBoxLayout(this);

    auto *title = new QLabel(QStringLiteral("ADD FFLAGS"), this);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet(QStringLiteral(
        "color:#FF5555;font-weight:bold;font-size:16px;letter-spacing:2px;border:none;"));
    layout->addWidget(title);

    editor_ = new QPlainTextEdit(this);
    editor_->setPlaceholderText(QStringLiteral(
        "Paste a JSON object here...\nExample:\n{\n  \"SomeFlag\": true,\n  \"SomeInt\": 120\n}"));
    editor_->setStyleSheet(QStringLiteral("font-family: Consolas, monospace;"));
    layout->addWidget(editor_, 1);

    auto *buttons = new QHBoxLayout();
    auto *add = new QPushButton(QStringLiteral("ADD"), this);
    auto *cancel = new QPushButton(QStringLiteral("CANCEL"), this);
    buttons->addWidget(add);
    buttons->addWidget(cancel);
    layout->addLayout(buttons);

    auto *import = new QPushButton(QStringLiteral("IMPORT FROM FILE"), this);
    import->setFixedHeight(35);
    layout->addWidget(import);

    connect(add, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(import, &QPushButton::clicked, this, [this] { importFromFile(); });
}

QJsonObject AddFlagDialog::flags(QString *error) const
{
    return parseJsonObject(editor_->toPlainText(), error);
}

void AddFlagDialog::importFromFile()
{
    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("Choose a JSON file"), {}, QStringLiteral("JSON Files (*.json)"));
    if (path.isEmpty())
        return;

    QFile file(path);
    if (file.size() > kMaxImportBytes) {
        QMessageBox::warning(this, QStringLiteral("Import failed"),
                             QStringLiteral("The file is too large (5 MB limit)."));
        return;
    }
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, QStringLiteral("Import failed"), file.errorString());
        return;
    }
    editor_->setPlainText(QString::fromUtf8(file.readAll()));
}

} // namespace SimpleInjector::Ui
