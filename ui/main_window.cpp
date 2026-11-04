#include "main_window.h"

#include "add_flag_dialog.h"
#include "core/flag_applier.h"
#include "core/flag_utils.h"
#include "platform/process_monitor.h"
#include "style.h"
#include "title_bar.h"

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QCloseEvent>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QShortcut>
#include <QTableWidget>
#include <QUrl>
#include <QVBoxLayout>

namespace SimpleInjector::Ui {

namespace {
constexpr auto kOffsetsUrl =
    "https://raw.githubusercontent.com/duyvietnam20121/fflags-offset/refs/heads/main/FFlags.hpp";
constexpr int kMonitorIntervalMs = 1500;
constexpr int kSearchDebounceMs = 150;
constexpr int kAutoApplyDelayMs = 250;
const QString kAutoApplyKey = QStringLiteral("auto_apply");
} // namespace

MainWindow::MainWindow()
{
    setWindowTitle(QStringLiteral("Simple Injector"));
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(620, 760);

    paths_ = prepareStorage();
    flags_.setPath(paths_.flags);
    flags_.load();
    settings_ = readJsonObject(paths_.settings, {{kAutoApplyKey, false}});

    offsets_ = new OffsetsService(QUrl(QString::fromLatin1(kOffsetsUrl)), paths_.offsets, this);
    offsets_->loadCache();
    monitor_ = new ProcessMonitor(kMonitorIntervalMs, this);

    searchDebounce_.setSingleShot(true);
    searchDebounce_.setInterval(kSearchDebounceMs);

    buildUi();
    connectSignals();

    refreshTable();
    updateOffsetsLabel();
    updateControls();
    offsets_->refresh();
    monitor_->start();
}

void MainWindow::buildUi()
{
    auto *container = new QWidget(this);
    container->setObjectName(QStringLiteral("MainContainer"));
    container->setStyleSheet(appStyleSheet());
    setCentralWidget(container);

    auto *root = new QVBoxLayout(container);
    root->setContentsMargins(1, 1, 1, 1);
    root->setSpacing(0);
    root->addWidget(new TitleBar(this));

    auto *content = new QVBoxLayout();
    content->setContentsMargins(20, 8, 20, 20);
    content->setSpacing(10);
    root->addLayout(content, 1);

    const QString dimLabel = QStringLiteral("color:#777;font-size:10px;border:none;");

    statusLabel_ = new QLabel(QStringLiteral("Trạng thái: đang kiểm tra Roblox..."), this);
    statusLabel_->setStyleSheet(statusStyle(false));
    content->addWidget(statusLabel_);

    offsetsLabel_ = new QLabel(this);
    offsetsLabel_->setStyleSheet(dimLabel);
    content->addWidget(offsetsLabel_);

    countLabel_ = new QLabel(this);
    countLabel_->setStyleSheet(dimLabel);
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
    search_->setPlaceholderText(QStringLiteral("Tìm flag theo tên hoặc giá trị..."));
    search_->setClearButtonEnabled(true);
    content->addWidget(search_);

    table_ = new QTableWidget(0, 2, this);
    table_->setHorizontalHeaderLabels({QStringLiteral("NAME"), QStringLiteral("VALUE")});
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->verticalHeader()->setVisible(false);
    table_->setShowGrid(false);
    table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    table_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    content->addWidget(table_, 1);

    auto *bottom = new QHBoxLayout();
    bottom->addStretch();
    auto *controls = new QVBoxLayout();
    autoApplyCheck_ = new QCheckBox(QStringLiteral("Auto Apply when Roblox starts"), this);
    autoApplyCheck_->setChecked(settings_.value(kAutoApplyKey).toBool());
    applyButton_ = new QPushButton(QStringLiteral("Apply All"), this);
    applyButton_->setObjectName(QStringLiteral("ApplyBtn"));
    applyButton_->setFixedSize(150, 34);
    controls->addWidget(autoApplyCheck_, 0, Qt::AlignRight);
    controls->addWidget(applyButton_);
    bottom->addLayout(controls);
    content->addLayout(bottom);
}

void MainWindow::connectSignals()
{
    connect(addButton_, &QPushButton::clicked, this, [this] { showAddDialog(); });
    connect(removeButton_, &QPushButton::clicked, this, [this] { removeSelected(); });
    connect(exportButton_, &QPushButton::clicked, this, [this] { exportFlags(); });
    connect(applyButton_, &QPushButton::clicked, this, [this] { applyAll(); });

    connect(search_, &QLineEdit::textChanged, &searchDebounce_, qOverload<>(&QTimer::start));
    connect(&searchDebounce_, &QTimer::timeout, this, [this] { refreshTable(); });

    connect(autoApplyCheck_, &QCheckBox::toggled, this, [this](bool checked) {
        settings_.insert(kAutoApplyKey, checked);
        saveSettings();
    });

    auto *copyShortcut = new QShortcut(QKeySequence::Copy, table_);
    copyShortcut->setContext(Qt::WidgetShortcut);
    connect(copyShortcut, &QShortcut::activated, this, [this] { copySelected(); });

    auto *deleteShortcut = new QShortcut(QKeySequence::Delete, table_);
    deleteShortcut->setContext(Qt::WidgetShortcut);
    connect(deleteShortcut, &QShortcut::activated, this, [this] { removeSelected(); });

    connect(monitor_, &ProcessMonitor::statusChanged, this, &MainWindow::onProcessStatusChanged);
    connect(monitor_, &ProcessMonitor::attached, this, [this] {
        if (!autoApplyCheck_->isChecked())
            return;
        // Chờ một chút để Roblox khởi tạo xong rồi mới ghi.
        QTimer::singleShot(kAutoApplyDelayMs, this, [this] {
            if (monitor_->isConnected())
                applyAll();
        });
    });

    connect(offsets_, &OffsetsService::refreshStarted, this, [this] {
        updateOffsetsLabel();
    });
    connect(offsets_, &OffsetsService::refreshFinished, this, &MainWindow::onOffsetsFinished);
}

// ---- Thao tác người dùng ---------------------------------------------------

void MainWindow::showAddDialog()
{
    AddFlagDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    QString error;
    const QJsonObject incoming = dialog.flags(&error);
    if (!error.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("JSON không hợp lệ"), error);
        return;
    }
    if (incoming.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("Không có flag"),
                                 QStringLiteral("JSON object đang rỗng."));
        return;
    }

    const FlagStore::MergeResult result = flags_.merge(incoming);
    if (result.added == 0) {
        QMessageBox::warning(this, QStringLiteral("Không có flag hợp lệ"),
                             QStringLiteral("Chỉ hỗ trợ giá trị kiểu chuỗi, số và boolean."));
        return;
    }

    refreshTable();
    QString saveError;
    if (!flags_.save(&saveError)) {
        setStatus(QStringLiteral("Không lưu được flags: %1").arg(saveError), false);
        return;
    }
    setStatus(result.skipped > 0
                  ? QStringLiteral("Đã thêm/cập nhật %1 flag · bỏ qua %2 mục không hợp lệ")
                        .arg(result.added).arg(result.skipped)
                  : QStringLiteral("Đã thêm/cập nhật %1 flag").arg(result.added),
              true);
    updateControls();
}

void MainWindow::removeSelected()
{
    const QStringList names = selectedNames();
    if (names.isEmpty())
        return;

    const int removed = flags_.remove(names);
    refreshTable();

    QString saveError;
    if (!flags_.save(&saveError)) {
        setStatus(QStringLiteral("Không lưu được flags: %1").arg(saveError), false);
        return;
    }
    setStatus(QStringLiteral("Đã xóa %1 flag").arg(removed), true);
    updateControls();
}

void MainWindow::exportFlags()
{
    const QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("Xuất flags"), QStringLiteral("fflags.json"),
        QStringLiteral("JSON Files (*.json)"));
    if (path.isEmpty())
        return;

    QString error;
    if (!writeJsonAtomically(path, flags_.entries(), &error)) {
        QMessageBox::warning(this, QStringLiteral("Xuất thất bại"), error);
        return;
    }
    setStatus(QStringLiteral("Đã xuất %1 flag").arg(flags_.size()), true);
}

void MainWindow::copySelected()
{
    const QJsonObject selected = flags_.subset(selectedNames());
    if (selected.isEmpty())
        return;
    QApplication::clipboard()->setText(
        QString::fromUtf8(QJsonDocument(selected).toJson(QJsonDocument::Indented)));
}

void MainWindow::applyAll()
{
    if (!monitor_->isConnected()) {
        setStatus(QStringLiteral("Chưa kết nối Roblox"), false);
        return;
    }
    if (offsets_->isEmpty()) {
        setStatus(QStringLiteral("Chưa có offsets; đã hủy Apply"), false);
        return;
    }

    RobloxProcess &process = monitor_->process();
    const ApplyReport report =
        applyFlags(flags_.entries(), offsets_->offsets(), process.imageBase(), process);
    setStatus(report.summary(), report.applied > 0);
}

// ---- Phản hồi từ service ---------------------------------------------------

void MainWindow::onProcessStatusChanged(const RobloxProcess::Status &status)
{
    using State = RobloxProcess::State;
    switch (status.state) {
    case State::Connected:
        setStatus(QStringLiteral("Đã kết nối Roblox · PID %1").arg(status.pid), true);
        break;
    case State::NotRunning:
        setStatus(QStringLiteral("Đang chờ RobloxPlayerBeta.exe"), false);
        break;
    case State::AccessDenied:
        setStatus(QStringLiteral("Thấy Roblox nhưng Windows từ chối truy cập (lỗi %1)")
                      .arg(status.errorCode), false);
        break;
    case State::NoModuleBase:
        setStatus(QStringLiteral("Đã mở Roblox nhưng không đọc được địa chỉ module chính"), false);
        break;
    case State::Unsupported:
        setStatus(QStringLiteral("Tính năng ghi bộ nhớ chỉ hỗ trợ Windows"), false);
        break;
    }
    updateControls();
}

void MainWindow::onOffsetsFinished(OffsetsService::Outcome outcome, const QString &detail)
{
    updateOffsetsLabel();
    updateControls();

    const bool connected = monitor_->isConnected();
    switch (outcome) {
    case OffsetsService::Outcome::Updated:
        setStatus(connected ? QStringLiteral("Đã kết nối Roblox · đã cập nhật offsets")
                            : QStringLiteral("Đã cập nhật offsets · đang chờ Roblox"), connected);
        break;
    case OffsetsService::Outcome::NetworkError:
        setStatus(QStringLiteral("Không tải được offsets, dùng bản cache (%1)").arg(detail), false);
        break;
    case OffsetsService::Outcome::EmptyResult:
    case OffsetsService::Outcome::TooLarge:
        setStatus(QStringLiteral("Nguồn offsets không dùng được: %1").arg(detail), false);
        break;
    }
}

// ---- Hiển thị --------------------------------------------------------------

void MainWindow::refreshTable()
{
    const QString query = search_->text().trimmed();
    const QJsonObject &entries = flags_.entries();

    table_->setUpdatesEnabled(false);
    table_->setRowCount(0);
    for (auto it = entries.constBegin(); it != entries.constEnd(); ++it) {
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

    countLabel_->setText(QStringLiteral("Flags đã lưu: %1  ·  Đang hiển thị: %2")
                             .arg(flags_.size()).arg(table_->rowCount()));
}

void MainWindow::updateOffsetsLabel()
{
    offsetsLabel_->setText(QStringLiteral("Offsets: %1 đã nạp  ·  nguồn: fflags.hpp%2")
                               .arg(offsets_->offsets().size())
                               .arg(offsets_->isRefreshing() ? QStringLiteral("  ·  đang cập nhật...")
                                                             : QString()));
}

void MainWindow::updateControls()
{
    applyButton_->setEnabled(monitor_->isConnected() && !offsets_->isEmpty() && !flags_.isEmpty());
}

void MainWindow::setStatus(const QString &message, bool good)
{
    statusLabel_->setText(QStringLiteral("Trạng thái: ") + message);
    statusLabel_->setStyleSheet(statusStyle(good));
}

QStringList MainWindow::selectedNames() const
{
    QStringList names;
    for (const QModelIndex &index : table_->selectionModel()->selectedRows()) {
        if (const QTableWidgetItem *item = table_->item(index.row(), 0))
            names.append(item->text());
    }
    return names;
}

void MainWindow::saveSettings()
{
    QString error;
    if (!writeJsonAtomically(paths_.settings, settings_, &error))
        setStatus(QStringLiteral("Không lưu được cài đặt: %1").arg(error), false);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    flags_.save();
    saveSettings();
    monitor_->process().detach();
    QMainWindow::closeEvent(event);
}

} // namespace SimpleInjector::Ui
