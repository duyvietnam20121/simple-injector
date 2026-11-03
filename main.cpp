#include <QApplication>
#include "flag_utils.h"
#include <QCheckBox>
#include <QClipboard>
#include <QDebug>
#include <QDialog>
#include <QCloseEvent>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QHeaderView>
#include <QHash>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMessageBox>
#include <QMouseEvent>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSaveFile>
#include <QShortcut>
#include <QStandardPaths>
#include <QStringList>
#include <QTableWidget>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <utility>
#include <cwchar>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <TlHelp32.h>
#endif

namespace {
constexpr auto kOffsetsUrl = "https://raw.githubusercontent.com/azayan165-svg/fflags.hpp/refs/heads/main/fflags.hpp";
constexpr auto kProcessName = L"RobloxPlayerBeta.exe";
constexpr int kMonitorIntervalMs = 1500;

const QString kStyle = QStringLiteral(R"(
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
)" );

using EzFlag::cleanPrefix;
using EzFlag::jsonValueToString;
using EzFlag::parseInt32Value;
using EzFlag::parseOffsetsHeader;
using EzFlag::readJsonObject;
using EzFlag::writeJsonAtomically;

#ifdef Q_OS_WIN
struct ProcessInfo {
    DWORD pid = 0;
    quintptr imageBase = 0;
};

std::optional<ProcessInfo> findRobloxProcess()
{
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return std::nullopt;

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    std::optional<DWORD> pid;

    if (Process32FirstW(snapshot, &entry)) {
        do {
            if (_wcsicmp(entry.szExeFile, kProcessName) == 0) {
                pid = entry.th32ProcessID;
                break;
            }
        } while (Process32NextW(snapshot, &entry));
    }
    CloseHandle(snapshot);

    if (!pid)
        return std::nullopt;

    HANDLE moduleSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, *pid);
    if (moduleSnapshot == INVALID_HANDLE_VALUE)
        return ProcessInfo{*pid, 0};

    MODULEENTRY32W module{};
    module.dwSize = sizeof(module);
    quintptr base = 0;
    if (Module32FirstW(moduleSnapshot, &module)) {
        do {
            if (_wcsicmp(module.szModule, kProcessName) == 0) {
                base = reinterpret_cast<quintptr>(module.modBaseAddr);
                break;
            }
        } while (Module32NextW(moduleSnapshot, &module));
    }
    CloseHandle(moduleSnapshot);
    return ProcessInfo{*pid, base};
}
#endif

class AddFlagDialog final : public QDialog {
public:
    explicit AddFlagDialog(QWidget *parent = nullptr) : QDialog(parent)
    {
        setWindowTitle(QStringLiteral("Add FFlags"));
        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
        setMinimumSize(500, 420);
        setStyleSheet(kStyle);

        auto *layout = new QVBoxLayout(this);
        auto *title = new QLabel(QStringLiteral("ADD FFLAGS"), this);
        title->setAlignment(Qt::AlignCenter);
        title->setStyleSheet(QStringLiteral("color:#FF5555;font-weight:bold;font-size:16px;letter-spacing:2px;border:none;"));
        layout->addWidget(title);

        editor_ = new QPlainTextEdit(this);
        editor_->setPlaceholderText(QStringLiteral("Paste a JSON object here...\nExample:\n{\n  \"SomeFlag\": true,\n  \"SomeInt\": 120\n}"));
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
        connect(import, &QPushButton::clicked, this, [this] {
            const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("Select JSON file"), {}, QStringLiteral("JSON Files (*.json)"));
            if (path.isEmpty()) return;
            QFile file(path);
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QMessageBox::warning(this, QStringLiteral("Import failed"), file.errorString());
                return;
            }
            editor_->setPlainText(QString::fromUtf8(file.readAll()));
        });
    }

    QJsonObject flags(QString *error) const
    {
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(editor_->toPlainText().toUtf8(), &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            if (error) *error = QStringLiteral("JSON error at offset %1: %2").arg(parseError.offset).arg(parseError.errorString());
            return {};
        }
        if (!doc.isObject()) {
            if (error) *error = QStringLiteral("The JSON root must be an object: { \"FlagName\": value }");
            return {};
        }
        return doc.object();
    }

private:
    QPlainTextEdit *editor_ = nullptr;
};

class TitleBar final : public QWidget {
public:
    explicit TitleBar(QMainWindow *window) : QWidget(window), window_(window)
    {
        auto *layout = new QHBoxLayout(this);
        layout->setContentsMargins(12, 5, 8, 5);
        auto *label = new QLabel(QStringLiteral("SIMPLE INJECTOR"), this);
        label->setStyleSheet(QStringLiteral("color:#B8B8B8;font-size:10px;font-weight:bold;letter-spacing:1px;border:none;"));
        auto *minimize = new QPushButton(QStringLiteral("—"), this);
        auto *close = new QPushButton(QStringLiteral("×"), this);
        for (QPushButton *button : {minimize, close}) {
            button->setFixedSize(30, 27);
            button->setStyleSheet(QStringLiteral("QPushButton{background:transparent;border:none;font-size:15px;} QPushButton:hover{color:#FF5555;}"));
        }
        layout->addWidget(label);
        layout->addStretch();
        layout->addWidget(minimize);
        layout->addWidget(close);
        connect(minimize, &QPushButton::clicked, window_, &QWidget::showMinimized);
        connect(close, &QPushButton::clicked, window_, &QWidget::close);
    }

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton) {
            dragOffset_ = event->globalPosition().toPoint() - window_->frameGeometry().topLeft();
            dragging_ = true;
        }
    }
    void mouseMoveEvent(QMouseEvent *event) override
    {
        if (dragging_ && (event->buttons() & Qt::LeftButton))
            window_->move(event->globalPosition().toPoint() - dragOffset_);
    }
    void mouseReleaseEvent(QMouseEvent *) override { dragging_ = false; }

private:
    QMainWindow *window_ = nullptr;
    QPoint dragOffset_;
    bool dragging_ = false;
};

class InjectorApp final : public QMainWindow {
public:
    InjectorApp()
    {
        setWindowTitle(QStringLiteral("Simple Injector"));
        setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
        setAttribute(Qt::WA_TranslucentBackground);
        setFixedSize(620, 760);

        QString dataRoot = qEnvironmentVariable("LOCALAPPDATA");
        if (dataRoot.isEmpty())
            dataRoot = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
        const QString legacyDataDir = QDir(dataRoot).filePath(QStringLiteral("ez"));
        dataDir_ = QDir(dataRoot).filePath(QStringLiteral("SimpleInjector"));
        QDir().mkpath(dataDir_);

        // Preserve data created by the earlier EZ-branded build.
        for (const QString &fileName : {QStringLiteral("ffs.json"),
                                        QStringLiteral("settings.json"),
                                        QStringLiteral("offsets.json")}) {
            const QString newPath = QDir(dataDir_).filePath(fileName);
            const QString oldPath = QDir(legacyDataDir).filePath(fileName);
            if (!QFile::exists(newPath) && QFile::exists(oldPath))
                QFile::copy(oldPath, newPath);
        }

        flagsPath_ = QDir(dataDir_).filePath(QStringLiteral("ffs.json"));
        settingsPath_ = QDir(dataDir_).filePath(QStringLiteral("settings.json"));
        offsetsCachePath_ = QDir(dataDir_).filePath(QStringLiteral("offsets.json"));

        flags_ = readJsonObject(flagsPath_, {});
        settings_ = readJsonObject(settingsPath_, {{QStringLiteral("auto_apply"), false}});
        offsets_ = loadOffsetsCache();

        auto *container = new QWidget(this);
        container->setObjectName(QStringLiteral("MainContainer"));
        container->setStyleSheet(kStyle);
        setCentralWidget(container);

        auto *root = new QVBoxLayout(container);
        root->setContentsMargins(1, 1, 1, 1);
        root->setSpacing(0);
        root->addWidget(new TitleBar(this));

        auto *content = new QVBoxLayout();
        content->setContentsMargins(20, 8, 20, 20);
        content->setSpacing(10);
        root->addLayout(content, 1);

        statusLabel_ = new QLabel(QStringLiteral("Status: checking Roblox..."), this);
        statusLabel_->setStyleSheet(QStringLiteral("color:#FF6666;font-weight:bold;font-size:11px;border:none;"));
        content->addWidget(statusLabel_);

        offsetsLabel_ = new QLabel(QStringLiteral("Offsets: loading..."), this);
        offsetsLabel_->setStyleSheet(QStringLiteral("color:#777;font-size:10px;border:none;"));
        content->addWidget(offsetsLabel_);

        countLabel_ = new QLabel(this);
        countLabel_->setStyleSheet(QStringLiteral("color:#777;font-size:10px;border:none;"));
        content->addWidget(countLabel_);

        auto *actions = new QHBoxLayout();
        addButton_ = new QPushButton(QStringLiteral("Add FFlag"), this);
        removeButton_ = new QPushButton(QStringLiteral("Remove selected"), this);
        exportButton_ = new QPushButton(QStringLiteral("Export"), this);
        for (QPushButton *button : {addButton_, removeButton_, exportButton_}) {
            button->setFixedHeight(34);
            actions->addWidget(button);
        }
        content->addLayout(actions);

        search_ = new QLineEdit(this);
        search_->setPlaceholderText(QStringLiteral("Search flags by name or value..."));
        content->addWidget(search_);

        table_ = new QTableWidget(0, 2, this);
        table_->setHorizontalHeaderLabels({QStringLiteral("NAME"), QStringLiteral("VALUE")});
        table_->setSelectionBehavior(QAbstractItemView::SelectRows);
        table_->setSelectionMode(QAbstractItemView::ExtendedSelection);
        table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table_->verticalHeader()->setVisible(false);
        table_->setShowGrid(false);
        table_->setSortingEnabled(false);
        table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        table_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        content->addWidget(table_, 1);

        auto *bottom = new QHBoxLayout();
        bottom->addStretch();
        auto *controls = new QVBoxLayout();
        autoApplyCheck_ = new QCheckBox(QStringLiteral("Auto Apply when Roblox starts"), this);
        autoApplyCheck_->setChecked(settings_.value(QStringLiteral("auto_apply")).toBool());
        applyButton_ = new QPushButton(QStringLiteral("Apply All"), this);
        applyButton_->setObjectName(QStringLiteral("ApplyBtn"));
        applyButton_->setFixedSize(150, 34);
        controls->addWidget(autoApplyCheck_, 0, Qt::AlignRight);
        controls->addWidget(applyButton_);
        bottom->addLayout(controls);
        content->addLayout(bottom);

        connect(addButton_, &QPushButton::clicked, this, [this] { showAddDialog(); });
        connect(removeButton_, &QPushButton::clicked, this, [this] { removeSelected(); });
        connect(exportButton_, &QPushButton::clicked, this, [this] { exportFlags(); });
        connect(search_, &QLineEdit::textChanged, this, [this] { refreshTable(); });
        connect(autoApplyCheck_, &QCheckBox::toggled, this, [this](bool checked) {
            settings_.insert(QStringLiteral("auto_apply"), checked);
            saveSettings();
        });
        connect(applyButton_, &QPushButton::clicked, this, [this] { applyAll(); });
        auto *copyShortcut = new QShortcut(QKeySequence::Copy, this);
        connect(copyShortcut, &QShortcut::activated, this, [this] { copySelected(); });

        network_ = new QNetworkAccessManager(this);
        connect(network_, &QNetworkAccessManager::finished, this, [this](QNetworkReply *reply) { offsetsReplyFinished(reply); });

        monitorTimer_ = new QTimer(this);
        monitorTimer_->setInterval(kMonitorIntervalMs);
        connect(monitorTimer_, &QTimer::timeout, this, [this] { checkProcess(); });

        refreshTable();
        updateControls();
        updateOffsetsLabel();
        fetchOffsets();
        checkProcess();
        monitorTimer_->start();
    }

    ~InjectorApp() override { closeProcessHandle(); }

protected:
    void closeEvent(QCloseEvent *event) override
    {
        saveFlags();
        saveSettings();
        closeProcessHandle();
        QMainWindow::closeEvent(event);
    }

private:
    QString dataDir_;
    QString flagsPath_;
    QString settingsPath_;
    QString offsetsCachePath_;
    QJsonObject flags_;
    QJsonObject settings_;
    QHash<QString, quintptr> offsets_;
    QNetworkAccessManager *network_ = nullptr;
    QTimer *monitorTimer_ = nullptr;
    QLabel *statusLabel_ = nullptr;
    QLabel *offsetsLabel_ = nullptr;
    QLabel *countLabel_ = nullptr;
    QLineEdit *search_ = nullptr;
    QTableWidget *table_ = nullptr;
    QCheckBox *autoApplyCheck_ = nullptr;
    QPushButton *addButton_ = nullptr;
    QPushButton *removeButton_ = nullptr;
    QPushButton *exportButton_ = nullptr;
    QPushButton *applyButton_ = nullptr;
    bool offsetsRequestInFlight_ = false;
    bool connected_ = false;
    quint32 processId_ = 0;
    quintptr imageBase_ = 0;
#ifdef Q_OS_WIN
    HANDLE processHandle_ = nullptr;
#endif

    QHash<QString, quintptr> loadOffsetsCache() const
    {
        QHash<QString, quintptr> result;
        const QJsonObject cached = readJsonObject(offsetsCachePath_, {});
        for (auto it = cached.begin(); it != cached.end(); ++it) {
            bool ok = false;
            const qulonglong value = it.value().toString().toULongLong(&ok, 16);
            if (ok && value != 0)
                result.insert(it.key(), static_cast<quintptr>(value));
        }
        return result;
    }

    void fetchOffsets()
    {
        if (offsetsRequestInFlight_) return;
        offsetsRequestInFlight_ = true;
        statusLabel_->setText(connected_ ? QStringLiteral("Status: connected · updating offsets...")
                                         : QStringLiteral("Status: checking Roblox · updating offsets..."));
        QNetworkRequest request{QUrl(QString::fromLatin1(kOffsetsUrl))};
        request.setTransferTimeout(10000);
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
        network_->get(request);
    }

    void offsetsReplyFinished(QNetworkReply *reply)
    {
        offsetsRequestInFlight_ = false;
        const QByteArray body = reply->readAll();
        const QNetworkReply::NetworkError networkError = reply->error();
        const QString errorText = reply->errorString();
        reply->deleteLater();

        if (networkError != QNetworkReply::NoError) {
            updateOffsetsLabel();
            setStatus(connected_ ? QStringLiteral("Connected; using cached offsets")
                                 : QStringLiteral("Waiting for Roblox; using cached offsets"), connected_);
            if (!offsets_.isEmpty())
                QMessageBox::information(this, QStringLiteral("Offsets"),
                    QStringLiteral("Could not refresh offsets. Cached offsets will be used.\n\n%1").arg(errorText));
            updateControls();
            return;
        }

        const QString source = QString::fromUtf8(body);
        const QHash<QString, quintptr> parsed = parseOffsetsHeader(source);

        if (parsed.isEmpty()) {
            updateOffsetsLabel();
            setStatus(QStringLiteral("Offsets source returned no valid entries"), false);
            updateControls();
            return;
        }

        offsets_ = std::move(parsed);
        QJsonObject cache;
        for (auto it = offsets_.cbegin(); it != offsets_.cend(); ++it)
            cache.insert(it.key(), QString::number(static_cast<qulonglong>(it.value()), 16));
        QString saveError;
        if (!writeJsonAtomically(offsetsCachePath_, cache, &saveError))
            qWarning("Could not save offset cache: %s", qPrintable(saveError));

        updateOffsetsLabel();
        setStatus(connected_ ? QStringLiteral("Roblox connected · offsets updated")
                             : QStringLiteral("Offsets updated · waiting for Roblox"), connected_);
        updateControls();
    }

    void checkProcess()
    {
#ifdef Q_OS_WIN
        const auto found = findRobloxProcess();
        if (!found) {
            if (connected_ || processHandle_) {
                closeProcessHandle();
                connected_ = false;
                processId_ = 0;
                imageBase_ = 0;
                setStatus(QStringLiteral("Waiting for RobloxPlayerBeta.exe"), false);
                updateControls();
            }
            return;
        }

        if (connected_ && found->pid == processId_ && processHandle_)
            return;

        closeProcessHandle();
        connected_ = false;
        processId_ = found->pid;
        imageBase_ = found->imageBase;
        processHandle_ = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION,
                                     FALSE, found->pid);
        if (!processHandle_) {
            setStatus(QStringLiteral("Roblox found, but Windows denied process access (error %1)").arg(GetLastError()), false);
            updateControls();
            return;
        }
        if (imageBase_ == 0) {
            setStatus(QStringLiteral("Roblox opened, but its main module base could not be read"), false);
            updateControls();
            return;
        }

        connected_ = true;
        setStatus(QStringLiteral("Connected to Roblox · PID %1").arg(processId_), true);
        updateControls();
        if (autoApplyCheck_->isChecked())
            QTimer::singleShot(250, this, [this] { applyAll(); });
#else
        setStatus(QStringLiteral("Process memory features are available on Windows only"), false);
#endif
    }

    void closeProcessHandle()
    {
#ifdef Q_OS_WIN
        if (processHandle_) {
            CloseHandle(processHandle_);
            processHandle_ = nullptr;
        }
#endif
        connected_ = false;
    }

    void applyAll()
    {
#ifdef Q_OS_WIN
        if (!connected_ || !processHandle_ || imageBase_ == 0) {
            setStatus(QStringLiteral("Roblox is not connected"), false);
            return;
        }
        if (offsets_.isEmpty()) {
            setStatus(QStringLiteral("No offsets are loaded; apply was cancelled"), false);
            return;
        }

        int applied = 0;
        int missingOffset = 0;
        int invalidValue = 0;
        int writeFailed = 0;

        for (auto it = flags_.cbegin(); it != flags_.cend(); ++it) {
            const QString name = cleanPrefix(it.key());
            const auto offsetIt = offsets_.constFind(name);
            if (offsetIt == offsets_.cend()) {
                ++missingOffset;
                continue;
            }

            qint32 value = 0;
            if (!parseInt32Value(jsonValueToString(it.value()), &value)) {
                ++invalidValue;
                continue;
            }

            const quintptr offset = offsetIt.value();
            if (offset > std::numeric_limits<quintptr>::max() - imageBase_) {
                ++writeFailed;
                continue;
            }
            const quintptr address = imageBase_ + offset;
            SIZE_T bytesWritten = 0;
            const BOOL success = WriteProcessMemory(processHandle_, reinterpret_cast<LPVOID>(address),
                                                    &value, sizeof(value), &bytesWritten);
            if (success && bytesWritten == sizeof(value))
                ++applied;
            else
                ++writeFailed;
        }

        setStatus(QStringLiteral("Apply finished · %1 written · %2 no offset · %3 unsupported value · %4 write errors")
                      .arg(applied).arg(missingOffset).arg(invalidValue).arg(writeFailed), applied > 0);
#else
        setStatus(QStringLiteral("Process memory features are available on Windows only"), false);
#endif
    }

    void showAddDialog()
    {
        AddFlagDialog dialog(this);
        if (dialog.exec() != QDialog::Accepted) return;

        QString error;
        const QJsonObject incoming = dialog.flags(&error);
        if (!error.isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("Invalid JSON"), error);
            return;
        }
        if (incoming.isEmpty()) {
            QMessageBox::information(this, QStringLiteral("No flags"), QStringLiteral("The JSON object is empty."));
            return;
        }

        int added = 0;
        for (auto it = incoming.begin(); it != incoming.end(); ++it) {
            bool valid = false;
            const QString value = jsonValueToString(it.value(), &valid);
            const QString name = cleanPrefix(it.key());
            if (!valid || name.isEmpty()) continue;
            flags_.insert(name, value);
            ++added;
        }
        if (added == 0) {
            QMessageBox::warning(this, QStringLiteral("No valid flags"),
                QStringLiteral("Only string, number, and boolean values are supported."));
            return;
        }
        refreshTable();
        saveFlags();
        setStatus(QStringLiteral("Added or updated %1 flag(s)").arg(added), true);
    }

    void removeSelected()
    {
        const QModelIndexList rows = table_->selectionModel()->selectedRows();
        if (rows.isEmpty()) return;
        QList<QString> names;
        for (const QModelIndex &index : rows) {
            const QTableWidgetItem *item = table_->item(index.row(), 0);
            if (item) names.append(item->text());
        }
        for (const QString &name : names) flags_.remove(name);
        refreshTable();
        saveFlags();
        setStatus(QStringLiteral("Removed %1 flag(s)").arg(names.size()), true);
    }

    void exportFlags()
    {
        const QString path = QFileDialog::getSaveFileName(this, QStringLiteral("Export flags"),
            QStringLiteral("fflags.json"), QStringLiteral("JSON Files (*.json)"));
        if (path.isEmpty()) return;
        QString error;
        if (!writeJsonAtomically(path, flags_, &error)) {
            QMessageBox::warning(this, QStringLiteral("Export failed"), error);
            return;
        }
        setStatus(QStringLiteral("Exported %1 flag(s)").arg(flags_.size()), true);
    }

    void copySelected()
    {
        QJsonObject selected;
        const QModelIndexList rows = table_->selectionModel()->selectedRows();
        for (const QModelIndex &index : rows) {
            const QTableWidgetItem *item = table_->item(index.row(), 0);
            if (!item) continue;
            const QString name = item->text();
            if (flags_.contains(name)) selected.insert(name, flags_.value(name));
        }
        if (!selected.isEmpty())
            QApplication::clipboard()->setText(QString::fromUtf8(QJsonDocument(selected).toJson(QJsonDocument::Indented)));
    }

    void refreshTable()
    {
        const QString query = search_ ? search_->text().trimmed() : QString();
        table_->setUpdatesEnabled(false);
        table_->setRowCount(0);
        for (auto it = flags_.cbegin(); it != flags_.cend(); ++it) {
            const QString name = it.key();
            const QString value = jsonValueToString(it.value());
            if (!query.isEmpty() && !name.contains(query, Qt::CaseInsensitive)
                && !value.contains(query, Qt::CaseInsensitive))
                continue;

            const int row = table_->rowCount();
            table_->insertRow(row);
            table_->setItem(row, 0, new QTableWidgetItem(name));
            table_->setItem(row, 1, new QTableWidgetItem(value));
        }
        table_->setUpdatesEnabled(true);
        countLabel_->setText(QStringLiteral("Saved FFlags: %1  ·  Showing: %2")
                             .arg(flags_.size()).arg(table_->rowCount()));
    }

    void updateOffsetsLabel()
    {
        offsetsLabel_->setText(QStringLiteral("Offsets: %1 loaded  ·  source: fflags.hpp")
                               .arg(offsets_.size()));
    }

    void updateControls()
    {
        applyButton_->setEnabled(connected_ && !offsets_.isEmpty() && !flags_.isEmpty());
    }

    void setStatus(const QString &message, bool good)
    {
        statusLabel_->setText(QStringLiteral("Status: ") + message);
        statusLabel_->setStyleSheet(good
            ? QStringLiteral("color:#61D786;font-weight:bold;font-size:11px;border:none;")
            : QStringLiteral("color:#FF6666;font-weight:bold;font-size:11px;border:none;"));
    }

    void saveFlags()
    {
        QString error;
        if (!writeJsonAtomically(flagsPath_, flags_, &error))
            setStatus(QStringLiteral("Could not save flags: %1").arg(error), false);
    }

    void saveSettings()
    {
        QString error;
        if (!writeJsonAtomically(settingsPath_, settings_, &error))
            setStatus(QStringLiteral("Could not save settings: %1").arg(error), false);
    }
};
} // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setStyle(QStringLiteral("Fusion"));
    QApplication::setApplicationName(QStringLiteral("Simple Injector"));
    QApplication::setOrganizationName(QStringLiteral("SimpleInjector"));

    InjectorApp window;
    window.show();
    return app.exec();
}
