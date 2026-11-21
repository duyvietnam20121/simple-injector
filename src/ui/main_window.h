#pragma once

#include "core/flags/flag_store.h"
#include "core/io/app_storage.h"
#include "core/offsets/roblox_version.h"
#include "platform/process_probe.h"
#include "platform/roblox_process.h"
#include "services/offsets_service.h"

#include <QJsonObject>
#include <QMainWindow>
#include <QTimer>

class QCheckBox;
class QCloseEvent;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableView;

namespace SimpleInjector {
class FlagFilterProxyModel;
class FlagTableModel;
class ProcessMonitor;
} // namespace SimpleInjector

namespace SimpleInjector::Ui {

/// Main window: coordinates the flag store, the offsets service and the Roblox monitor.
/// It holds no business logic; every decision lives in the core/service classes.
class MainWindow final : public QMainWindow {
public:
    MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void buildUi();
    void connectSignals();

    // User actions
    void showAddDialog();
    void removeSelected();
    void exportFlags();
    void copySelected();
    /// `interactive` is false when called by Auto Apply: no dialogs, and a version mismatch
    /// postpones the run instead of asking.
    void applyAll(bool interactive);

    // Reactions to services
    void onProcessStatusChanged(const ProcessStatus &status);
    void onOffsetsFinished(OffsetsService::Outcome outcome, const QString &detail);
    void onValueEdited(const QString &name, const QString &value);

    // Display
    void updateCount();
    void updateOffsetsLabel();
    void updateVersionLabel();
    void updateControls();
    void setStatus(const QString &message, bool good);
    QStringList selectedNames() const;
    VersionMatch currentVersionMatch() const;
    bool saveFlags(const QString &successMessage);
    void saveSettings();

    StoragePaths paths_;
    FlagStore flags_;
    QJsonObject settings_;
    RobloxProcess process_;
    OffsetsService *offsets_ = nullptr;
    ProcessMonitor *monitor_ = nullptr;
    FlagTableModel *model_ = nullptr;
    FlagFilterProxyModel *proxy_ = nullptr;
    QTimer searchDebounce_;
    bool pendingAutoApply_ = false; ///< Auto Apply postponed by a version mismatch, waiting for newer offsets.

    QLabel *statusLabel_ = nullptr;
    QLabel *offsetsLabel_ = nullptr;
    QLabel *versionLabel_ = nullptr;
    QLabel *countLabel_ = nullptr;
    QLineEdit *search_ = nullptr;
    QTableView *table_ = nullptr;
    QCheckBox *failedOnlyCheck_ = nullptr;
    QCheckBox *autoApplyCheck_ = nullptr;
    QPushButton *refreshOffsetsButton_ = nullptr;
    QPushButton *addButton_ = nullptr;
    QPushButton *removeButton_ = nullptr;
    QPushButton *exportButton_ = nullptr;
    QPushButton *applyButton_ = nullptr;
};

} // namespace SimpleInjector::Ui
