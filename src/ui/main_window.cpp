#include "ui/main_window.h"

#include "core/apply/flag_applier.h"
#include "core/flags/flag_filter_proxy_model.h"
#include "core/flags/flag_table_model.h"
#include "core/io/json_io.h"
#include "platform/process_monitor.h"
#include "ui/add_flag_dialog.h"
#include "ui/style.h"
#include "ui/title_bar.h"
#include "ui/window_sizing.h"

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QCloseEvent>
#include <QCoreApplication>
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
constexpr int kOffsetsRefreshMs = 15 * 60 * 1000; // refresh the offsets every 15 minutes
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
    setWindowTitle(QStringLiteral("Simple Injector v%1").arg(QCoreApplication::applicationVersion()));
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    setAttribute(Qt::WA_TranslucentBackground);

    // The window size follows the screen (it is not fixed) and has a resize grip at the bottom right.
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

    statusLabel_ = new QLabel(QStringLiteral("Status: checking for Roblox..."), this);
    statusLabel_->setStyleSheet(statusStyle(false));
    statusLabel_->setWordWrap(true);
    content->addWidget(statusLabel_);

    versionLabel_ = new QLabel(this);
    content->addWidget(versionLabel_);

    offsetsLabel_ = new QLabel(this);
    offsetsLabel_->setWordWrap(true);
    refreshOffsetsButton_ = new QPushButton(QStringLiteral("↻ Refresh"), this);
    refreshOffsetsButton_->setFixedSize(92, 26);
    refreshOffsetsButton_->setToolTip(QStringLiteral("Download the offsets table again now"));
    refreshOffsetsButton_->setStyleSheet(QStringLiteral("padding:2px 8px;font-size:11px;"));

    auto *offsetsRow = new QHBoxLayout();
    offsetsRow->addWidget(offsetsLabel_, 1);
    offsetsRow->addWidget(refreshOffsetsButton_, 0, Qt::AlignTop);
    content->addLayout(offsetsRow);

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
    search_->setPlaceholderText(QStringLiteral("Search by name, value or status..."));
    search_->setClearButtonEnabled(true);
    failedOnlyCheck_ = new QCheckBox(QStringLiteral("Only show failed flags"), this);

    auto *filterRow = new QHBoxLayout();
    filterRow->addWidget(search_, 1);
    filterRow->addWidget(failedOnlyCheck_);
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

    auto *hint = new QLabel(QStringLiteral("Double-click (or press F2) in the VALUE column to edit · Delete removes the selection"), this);
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
    connect(refreshOffsetsButton_, &QPushButton::clicked, this, [this] { offsets_->refresh(); });

    connect(search_, &QLineEdit::textChanged, &searchDebounce_, qOverload<>(&QTimer::start));
    connect(&searchDebounce_, &QTimer::timeout, this, [this] {
        proxy_->setFilterFixedString(search_->text().trimmed());
    });

    // The table changed (filter, add, remove, reset): update the counters.
    connect(proxy_, &QAbstractItemModel::rowsInserted, this, [this] { updateCount(); });
    connect(proxy_, &QAbstractItemModel::rowsRemoved, this, [this] { updateCount(); });
    connect(proxy_, &QAbstractItemModel::modelReset, this, [this] { updateCount(); });
    connect(model_, &QAbstractItemModel::dataChanged, this, [this] { updateCount(); });
    connect(failedOnlyCheck_, &QCheckBox::toggled, this,
            [this](bool checked) { proxy_->setFailedOnly(checked); });
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
        model_->clearApplyResults(); // a new process: the old apply results no longer hold
        // Roblox may have just updated: fetch the latest offsets if the versions differ.
        if (currentVersionMatch() == VersionMatch::Mismatch)
            offsets_->refresh();

        if (!autoApplyCheck_->isChecked())
            return;
        // Give Roblox a moment to finish starting up before writing.
        QTimer::singleShot(kAutoApplyDelayMs, this, [this] {
            if (monitor_->isConnected())
                applyAll(false);
        });
    });

    connect(offsets_, &OffsetsService::refreshStarted, this, [this] { updateOffsetsLabel(); });
    connect(offsets_, &OffsetsService::refreshFinished, this, &MainWindow::onOffsetsFinished);
}

// ---- User actions ----------------------------------------------------------

void MainWindow::showAddDialog()
{
    AddFlagDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    QString error;
    const QJsonObject incoming = dialog.flags(&error);
    if (!error.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Invalid JSON"), error);
        return;
    }
    if (incoming.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("No flags"),
                                 QStringLiteral("The JSON object is empty."));
        return;
    }

    const FlagStore::MergeResult result = flags_.merge(incoming);
    if (result.added == 0) {
        QMessageBox::warning(this, QStringLiteral("No valid flags"),
                             QStringLiteral("Only string, number and boolean values are supported."));
        return;
    }

    model_->reload();
    saveFlags(result.skipped > 0
                  ? QStringLiteral("Added/updated %1 flags · skipped %2 invalid entries")
                        .arg(result.added).arg(result.skipped)
                  : QStringLiteral("Added/updated %1 flags").arg(result.added));
}

void MainWindow::removeSelected()
{
    const QStringList names = selectedNames();
    if (names.isEmpty())
        return;

    const int removed = flags_.remove(names);
    model_->reload();
    saveFlags(QStringLiteral("Removed %1 flags").arg(removed));
}

void MainWindow::exportFlags()
{
    const QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("Export flags"), QStringLiteral("fflags.json"),
        QStringLiteral("JSON Files (*.json)"));
    if (path.isEmpty())
        return;

    QString error;
    if (!writeJsonAtomically(path, flags_.entries(), &error)) {
        QMessageBox::warning(this, QStringLiteral("Export failed"), error);
        return;
    }
    setStatus(QStringLiteral("Exported %1 flags").arg(flags_.size()), true);
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
        setStatus(QStringLiteral("Not connected to Roblox"), false);
        return;
    }
    if (offsets_->isEmpty()) {
        setStatus(QStringLiteral("No offsets available; Apply cancelled"), false);
        return;
    }

    if (currentVersionMatch() == VersionMatch::Mismatch) {
        const QString detail = QStringLiteral("Roblox is running %1 but the offsets belong to %2.")
                                   .arg(shortVersion(monitor_->status().version),
                                        shortVersion(offsets_->version()));
        if (!interactive) {
            // Do not write with offsets that are probably stale: wait for fresh ones and retry.
            pendingAutoApply_ = true;
            setStatus(QStringLiteral("Auto Apply postponed: %1 Checking for newer offsets...").arg(detail), false);
            offsets_->refresh();
            return;
        }
        const auto answer = QMessageBox::warning(
            this, QStringLiteral("Offsets may be outdated"),
            detail + QStringLiteral("\n\nWriting to a wrong offset can destabilize Roblox. Continue anyway?"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (answer != QMessageBox::Yes)
            return;
    }

    const ApplyReport report =
        applyFlags(flags_.entries(), offsets_->offsets(), process_.imageBase(), process_);
    model_->setApplyResults(report.results);

    const bool allApplied = report.total() > 0 && report.failed() == 0;
    setStatus(report.summary(), allApplied);
}

// ---- Reactions to services -------------------------------------------------

void MainWindow::onProcessStatusChanged(const ProcessStatus &status)
{
    if (status.state != ProcessState::Connected)
        model_->clearApplyResults(); // disconnected: "applied" no longer holds
    switch (status.state) {
    case ProcessState::Connected:
        setStatus(QStringLiteral("Connected to Roblox · PID %1").arg(status.pid), true);
        break;
    case ProcessState::NotRunning:
        pendingAutoApply_ = false;
        setStatus(QStringLiteral("Waiting for RobloxPlayerBeta.exe"), false);
        break;
    case ProcessState::AccessDenied:
        setStatus(QStringLiteral("Found Roblox but Windows denied access (error %1)")
                      .arg(status.errorCode), false);
        break;
    case ProcessState::NoModuleBase:
        setStatus(QStringLiteral("Opened Roblox but could not read the main module address"), false);
        break;
    case ProcessState::Unsupported:
        setStatus(QStringLiteral("Writing process memory is supported on Windows only"), false);
        break;
    }
    updateVersionLabel();
    updateControls();
}

void MainWindow::onOffsetsFinished(OffsetsService::Outcome outcome, const QString &detail)
{
    updateOffsetsLabel();
    updateVersionLabel();
    updateControls();

    const bool connected = monitor_->isConnected();
    const bool wasPending = std::exchange(pendingAutoApply_, false);

    switch (outcome) {
    case OffsetsService::Outcome::Updated: {
        const VersionMatch match = currentVersionMatch();
        if (connected && match == VersionMatch::Mismatch) {
            setStatus(QStringLiteral("Offsets updated but they still do not match the Roblox version. "
                                     "Wait for the offsets source to catch up with the new Roblox build."), false);
        } else if (wasPending && connected) {
            applyAll(false); // the new offsets match: finish the postponed Auto Apply
        } else {
            setStatus(connected ? QStringLiteral("Connected to Roblox · offsets updated")
                                : QStringLiteral("Offsets updated · waiting for Roblox"), connected);
        }
        break;
    }
    case OffsetsService::Outcome::NetworkError:
        setStatus(QStringLiteral("Could not download offsets, using the cache (%1)").arg(detail), false);
        break;
    case OffsetsService::Outcome::EmptyResult:
    case OffsetsService::Outcome::TooLarge:
        setStatus(QStringLiteral("The offsets source is unusable: %1").arg(detail), false);
        break;
    }
}

void MainWindow::onValueEdited(const QString &name, const QString &value)
{
    saveFlags(QStringLiteral("Edited %1 = %2").arg(name, value));
}

// ---- Display ---------------------------------------------------------------

void MainWindow::updateCount()
{
    const int failed = model_->failedCount();
    QString text = QStringLiteral("Saved flags: %1  ·  Showing: %2")
                       .arg(flags_.size()).arg(proxy_->rowCount());
    if (failed > 0)
        text += QStringLiteral("  ·  Failed: %1").arg(failed);
    countLabel_->setText(text);
    updateControls();
}

void MainWindow::updateOffsetsLabel()
{
    QString text = QStringLiteral("Offsets: %1 entries").arg(offsets_->offsets().size());
    if (!offsets_->version().isEmpty())
        text += QStringLiteral(" · %1").arg(offsets_->version());

    switch (offsets_->source()) {
    case OffsetsService::Source::Network:
        text += QStringLiteral(" · downloaded at %1").arg(offsets_->loadedAt().toString(QStringLiteral("HH:mm:ss")));
        break;
    case OffsetsService::Source::Cache:
        text += QStringLiteral(" · CACHE on disk (saved %1)")
                    .arg(offsets_->loadedAt().toString(QStringLiteral("MM-dd HH:mm")));
        break;
    case OffsetsService::Source::None:
        text += QStringLiteral(" · no data yet");
        break;
    }
    if (offsets_->isRefreshing())
        text += QStringLiteral(" · updating...");

    const bool failed = !offsets_->lastError().isEmpty();
    if (failed) {
        text += QStringLiteral("\n⚠ The most recent download failed: %1").arg(offsets_->lastError());
        if (offsets_->source() == OffsetsService::Source::Cache)
            text += QStringLiteral(" — using the cache; newer flags may be missing");
    }

    offsetsLabel_->setText(text);
    offsetsLabel_->setStyleSheet(versionStyle(failed ? QStringLiteral("#FFB347") : QStringLiteral("#777")));
    refreshOffsetsButton_->setEnabled(!offsets_->isRefreshing());
}

void MainWindow::updateVersionLabel()
{
    const VersionMatch match = currentVersionMatch();
    QString verdict;
    QString color;
    switch (match) {
    case VersionMatch::Match:
        verdict = QStringLiteral("✓ match");
        color = QStringLiteral("#61D786");
        break;
    case VersionMatch::Mismatch:
        verdict = QStringLiteral("⚠ MISMATCH - the offsets may be outdated");
        color = QStringLiteral("#FFB347");
        break;
    case VersionMatch::Unknown:
        verdict = QStringLiteral("cannot compare yet");
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
    statusLabel_->setText(QStringLiteral("Status: ") + message);
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
        setStatus(QStringLiteral("Could not save flags: %1").arg(error), false);
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
        setStatus(QStringLiteral("Could not save settings: %1").arg(error), false);
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
