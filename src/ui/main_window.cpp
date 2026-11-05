#include "main_window.h"

#include "add_flag_dialog.h"
#include "core/flag_applier.h"
#include "core/flag_table_model.h"
#include "core/flag_utils.h"
#include "core/window_sizing.h"
#include "platform/process_monitor.h"
#include "style.h"
#include "title_bar.h"

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QCloseEvent>
#include <QFileDialog>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QScreen>
#include <QShortcut>
#include <QSizeGrip>
#include <QTableView>
#include <QUrl>
#include <QVBoxLayout>

#include <utility>

namespace SimpleInjector::Ui {

namespace {
constexpr auto kOffsetsUrl =
    "https://raw.githubusercontent.com/duyvietnam20121/fflags-offset/refs/heads/main/FFlags.hpp";
constexpr int kMonitorIntervalMs = 1500;
constexpr int kOffsetsRefreshMs = 15 * 60 * 1000; // làm mới offsets mỗi 15 phút
constexpr int kSearchDebounceMs = 150;
constexpr int kAutoApplyDelayMs = 250;
const QString kAutoApplyKey = QStringLiteral("auto_apply");
const QSize kPreferredSize(620, 720);
const QSize kMinimumSize(520, 480);

QString shortVersion(const QString &version)
{
    return version.isEmpty() ? QStringLiteral("?") : version;
}
} // namespace

MainWindow::MainWindow()
{
    setWindowTitle(QStringLiteral("Simple Injector"));
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    setAttribute(Qt::WA_TranslucentBackground);

    // Cỡ cửa sổ co theo màn hình (không cố định) và có tay nắm kéo ở góc phải dưới.
    const QRect available = QGuiApplication::primaryScreen()->availableGeometry();
    setMinimumSize(fitToScreen(kMinimumSize, available));
    const QSize initial = fitToScreen(kPreferredSize, available);
    setGeometry(centeredIn(initial, available));

    paths_ = prepareStorage();
    flags_.setPath(paths_.flags);
    flags_.load();
    settings_ = readJsonObject(paths_.settings, {{kAutoApplyKey, false}});

    offsets_ = new OffsetsService(QUrl(QString::fromLatin1(kOffsetsUrl)), paths_.offsets, this);
    offsets_->loadCache();
    monitor_ = new ProcessMonitor(&process_, kMonitorIntervalMs, this);

    model_ = new FlagTableModel(&flags_, this);
    model_->setOffsets(&offsets_->offsets()); // để dự đoán trạng thái từng flag trước khi Apply
    proxy_ = new FlagFilterProxyModel(this);
    proxy_->setSourceModel(model_);

    searchDebounce_.setSingleShot(true);
    searchDebounce_.setInterval(kSearchDebounceMs);

    buildUi();
    connectSignals();

    updateCount();
    updateOffsetsLabel();
    updateVersionLabel();
    updateControls();
    offsets_->refresh();
    offsets_->startAutoRefresh(kOffsetsRefreshMs);
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
    statusLabel_->setWordWrap(true);
    content->addWidget(statusLabel_);

    versionLabel_ = new QLabel(this);
    content->addWidget(versionLabel_);

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
    search_->setPlaceholderText(QStringLiteral("Tìm theo tên, giá trị hoặc trạng thái..."));
    search_->setClearButtonEnabled(true);
    problemsOnlyCheck_ = new QCheckBox(QStringLiteral("Chỉ hiện flag có vấn đề"), this);

    auto *filterRow = new QHBoxLayout();
    filterRow->addWidget(search_, 1);
    filterRow->addWidget(problemsOnlyCheck_);
    content->addLayout(filterRow);

    table_ = new QTableView(this);
    table_->setModel(proxy_);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    table_->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    table_->verticalHeader()->setVisible(false);
    table_->verticalHeader()->setDefaultSectionSize(28);
    table_->setShowGrid(false);
    table_->setSortingEnabled(true);
    table_->sortByColumn(FlagTableModel::NameColumn, Qt::AscendingOrder);
    table_->setTextElideMode(Qt::ElideRight);
    QHeaderView *header = table_->horizontalHeader();
    header->setSectionResizeMode(FlagTableModel::NameColumn, QHeaderView::Stretch);
    header->setSectionResizeMode(FlagTableModel::ValueColumn, QHeaderView::Interactive);
    header->setSectionResizeMode(FlagTableModel::StatusColumn, QHeaderView::Interactive);
    header->resizeSection(FlagTableModel::ValueColumn, 110);
    header->resizeSection(FlagTableModel::StatusColumn, 190);
    content->addWidget(table_, 1);

    auto *hint = new QLabel(QStringLiteral("Nhấp đúp (hoặc F2) cột VALUE để sửa · rê chuột vào NAME/STATUS để xem lý do · Delete để xóa"), this);
    hint->setStyleSheet(QStringLiteral("color:#555;font-size:10px;border:none;"));
    content->addWidget(hint);

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
    bottom->addWidget(new QSizeGrip(this), 0, Qt::AlignBottom | Qt::AlignRight);
    content->addLayout(bottom);
}

void MainWindow::connectSignals()
{
    connect(addButton_, &QPushButton::clicked, this, [this] { showAddDialog(); });
    connect(removeButton_, &QPushButton::clicked, this, [this] { removeSelected(); });
    connect(exportButton_, &QPushButton::clicked, this, [this] { exportFlags(); });
    connect(applyButton_, &QPushButton::clicked, this, [this] { applyAll(true); });

    connect(search_, &QLineEdit::textChanged, &searchDebounce_, qOverload<>(&QTimer::start));
    connect(&searchDebounce_, &QTimer::timeout, this, [this] {
        proxy_->setFilterFixedString(search_->text().trimmed());
    });

    // Bảng đổi (lọc, thêm, xóa, reset) -> cập nhật bộ đếm.
    connect(proxy_, &QAbstractItemModel::rowsInserted, this, [this] { updateCount(); });
    connect(proxy_, &QAbstractItemModel::rowsRemoved, this, [this] { updateCount(); });
    connect(proxy_, &QAbstractItemModel::modelReset, this, [this] { updateCount(); });
    connect(model_, &QAbstractItemModel::dataChanged, this, [this] { updateCount(); });
    connect(problemsOnlyCheck_, &QCheckBox::toggled, this,
            [this](bool checked) { proxy_->setProblemsOnly(checked); });
    connect(model_, &FlagTableModel::valueEdited, this, &MainWindow::onValueEdited);

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
        model_->clearApplyResults(); // tiến trình mới: kết quả Apply cũ không còn đúng
        // Roblox có thể vừa cập nhật: lấy offsets mới nhất nếu đang lệch phiên bản.
        if (currentVersionMatch() == VersionMatch::Mismatch)
            offsets_->refresh();

        if (!autoApplyCheck_->isChecked())
            return;
        // Chờ một chút để Roblox khởi tạo xong rồi mới ghi.
        QTimer::singleShot(kAutoApplyDelayMs, this, [this] {
            if (monitor_->isConnected())
                applyAll(false);
        });
    });

    connect(offsets_, &OffsetsService::refreshStarted, this, [this] { updateOffsetsLabel(); });
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

    model_->reload();
    saveFlags(result.skipped > 0
                  ? QStringLiteral("Đã thêm/cập nhật %1 flag · bỏ qua %2 mục không hợp lệ")
                        .arg(result.added).arg(result.skipped)
                  : QStringLiteral("Đã thêm/cập nhật %1 flag").arg(result.added));
}

void MainWindow::removeSelected()
{
    const QStringList names = selectedNames();
    if (names.isEmpty())
        return;

    const int removed = flags_.remove(names);
    model_->reload();
    saveFlags(QStringLiteral("Đã xóa %1 flag").arg(removed));
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

void MainWindow::applyAll(bool interactive)
{
    if (!monitor_->isConnected()) {
        setStatus(QStringLiteral("Chưa kết nối Roblox"), false);
        return;
    }
    if (offsets_->isEmpty()) {
        setStatus(QStringLiteral("Chưa có offsets; đã hủy Apply"), false);
        return;
    }

    if (currentVersionMatch() == VersionMatch::Mismatch) {
        const QString detail = QStringLiteral("Roblox đang chạy %1 nhưng offsets thuộc %2.")
                                   .arg(shortVersion(monitor_->status().version),
                                        shortVersion(offsets_->version()));
        if (!interactive) {
            // Không tự ghi bằng offsets có khả năng đã cũ: chờ bản mới rồi thử lại.
            pendingAutoApply_ = true;
            setStatus(QStringLiteral("Auto Apply tạm hoãn: %1 Đang kiểm tra offsets mới...").arg(detail), false);
            offsets_->refresh();
            return;
        }
        const auto answer = QMessageBox::warning(
            this, QStringLiteral("Offsets có thể đã cũ"),
            detail + QStringLiteral("\n\nGhi sai offset có thể làm Roblox mất ổn định. Vẫn tiếp tục?"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (answer != QMessageBox::Yes)
            return;
    }

    const ApplyReport report =
        applyFlags(flags_.entries(), offsets_->offsets(), process_.imageBase(), process_);
    model_->setApplyResults(report.results);

    const int problems = report.verifyFailed + report.missingOffset + report.invalidValue + report.writeFailed;
    QString message = report.summary();
    if (problems > 0)
        message += QStringLiteral(" · xem cột STATUS để biết flag nào lỗi");
    setStatus(message, report.applied > 0 && report.verifyFailed == 0);
}

// ---- Phản hồi từ service ---------------------------------------------------

void MainWindow::onProcessStatusChanged(const ProcessStatus &status)
{
    if (status.state != ProcessState::Connected)
        model_->clearApplyResults(); // mất kết nối: trạng thái "đã áp dụng" không còn đúng
    switch (status.state) {
    case ProcessState::Connected:
        setStatus(QStringLiteral("Đã kết nối Roblox · PID %1").arg(status.pid), true);
        break;
    case ProcessState::NotRunning:
        pendingAutoApply_ = false;
        setStatus(QStringLiteral("Đang chờ RobloxPlayerBeta.exe"), false);
        break;
    case ProcessState::AccessDenied:
        setStatus(QStringLiteral("Thấy Roblox nhưng Windows từ chối truy cập (lỗi %1)")
                      .arg(status.errorCode), false);
        break;
    case ProcessState::NoModuleBase:
        setStatus(QStringLiteral("Đã mở Roblox nhưng không đọc được địa chỉ module chính"), false);
        break;
    case ProcessState::Unsupported:
        setStatus(QStringLiteral("Tính năng ghi bộ nhớ chỉ hỗ trợ Windows"), false);
        break;
    }
    updateVersionLabel();
    updateControls();
}

void MainWindow::onOffsetsFinished(OffsetsService::Outcome outcome, const QString &detail)
{
    model_->refreshStatuses(); // bảng offsets có thể đã đổi: tính lại dự đoán từng flag
    updateOffsetsLabel();
    updateVersionLabel();
    updateControls();

    const bool connected = monitor_->isConnected();
    const bool wasPending = std::exchange(pendingAutoApply_, false);

    switch (outcome) {
    case OffsetsService::Outcome::Updated: {
        const VersionMatch match = currentVersionMatch();
        if (connected && match == VersionMatch::Mismatch) {
            setStatus(QStringLiteral("Offsets đã cập nhật nhưng vẫn lệch phiên bản Roblox. "
                                     "Chờ nguồn offsets theo kịp bản Roblox mới."), false);
        } else if (wasPending && connected) {
            applyAll(false); // offsets mới đã khớp: hoàn tất Auto Apply bị hoãn
        } else {
            setStatus(connected ? QStringLiteral("Đã kết nối Roblox · đã cập nhật offsets")
                                : QStringLiteral("Đã cập nhật offsets · đang chờ Roblox"), connected);
        }
        break;
    }
    case OffsetsService::Outcome::NetworkError:
        setStatus(QStringLiteral("Không tải được offsets, dùng bản cache (%1)").arg(detail), false);
        break;
    case OffsetsService::Outcome::EmptyResult:
    case OffsetsService::Outcome::TooLarge:
        setStatus(QStringLiteral("Nguồn offsets không dùng được: %1").arg(detail), false);
        break;
    }
}

void MainWindow::onValueEdited(const QString &name, const QString &value)
{
    saveFlags(QStringLiteral("Đã sửa %1 = %2").arg(name, value));
}

// ---- Hiển thị --------------------------------------------------------------

void MainWindow::updateCount()
{
    const int problems = model_->problemCount();
    QString text = QStringLiteral("Flags đã lưu: %1  ·  Đang hiển thị: %2")
                       .arg(flags_.size()).arg(proxy_->rowCount());
    if (problems > 0)
        text += QStringLiteral("  ·  Không áp dụng được: %1").arg(problems);
    countLabel_->setText(text);
    updateControls();
}

void MainWindow::updateOffsetsLabel()
{
    offsetsLabel_->setText(QStringLiteral("Offsets: %1 đã nạp  ·  nguồn: fflags.hpp%2")
                               .arg(offsets_->offsets().size())
                               .arg(offsets_->isRefreshing() ? QStringLiteral("  ·  đang cập nhật...")
                                                             : QString()));
}

void MainWindow::updateVersionLabel()
{
    const VersionMatch match = currentVersionMatch();
    QString verdict;
    QString color;
    switch (match) {
    case VersionMatch::Match:
        verdict = QStringLiteral("✓ khớp");
        color = QStringLiteral("#61D786");
        break;
    case VersionMatch::Mismatch:
        verdict = QStringLiteral("⚠ KHÔNG KHỚP - offsets có thể đã cũ");
        color = QStringLiteral("#FFB347");
        break;
    case VersionMatch::Unknown:
        verdict = QStringLiteral("chưa đối chiếu được");
        color = QStringLiteral("#777");
        break;
    }
    versionLabel_->setText(QStringLiteral("Roblox: %1  ·  Offsets: %2  ·  %3")
                               .arg(shortVersion(monitor_->status().version),
                                    shortVersion(offsets_->version()), verdict));
    versionLabel_->setStyleSheet(versionStyle(color));
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
    for (const QModelIndex &index : table_->selectionModel()->selectedRows(FlagTableModel::NameColumn))
        names.append(index.data(Qt::DisplayRole).toString());
    return names;
}

VersionMatch MainWindow::currentVersionMatch() const
{
    if (!monitor_->isConnected())
        return VersionMatch::Unknown;
    return compareVersions(offsets_->version(), monitor_->status().version);
}

bool MainWindow::saveFlags(const QString &successMessage)
{
    QString error;
    if (!flags_.save(&error)) {
        setStatus(QStringLiteral("Không lưu được flags: %1").arg(error), false);
        return false;
    }
    setStatus(successMessage, true);
    updateControls();
    return true;
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
    monitor_->stop();
    process_.detach();
    QMainWindow::closeEvent(event);
}

} // namespace SimpleInjector::Ui
