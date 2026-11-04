#pragma once

#include "core/app_storage.h"
#include "core/flag_store.h"
#include "core/roblox_version.h"
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
class QSortFilterProxyModel;
class QTableView;

namespace SimpleInjector {
class FlagTableModel;
class ProcessMonitor;
} // namespace SimpleInjector

namespace SimpleInjector::Ui {

/// Cửa sổ chính: điều phối kho flag, dịch vụ offsets và trình theo dõi Roblox.
/// Không chứa logic nghiệp vụ; mọi quyết định nằm trong các lớp core/service.
class MainWindow final : public QMainWindow {
public:
    MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void buildUi();
    void connectSignals();

    // Thao tác người dùng
    void showAddDialog();
    void removeSelected();
    void exportFlags();
    void copySelected();
    /// `interactive` = false khi do Auto Apply gọi: không hiện hộp thoại, gặp lệch phiên bản thì dừng.
    void applyAll(bool interactive);

    // Phản hồi từ service
    void onProcessStatusChanged(const ProcessStatus &status);
    void onOffsetsFinished(OffsetsService::Outcome outcome, const QString &detail);
    void onValueEdited(const QString &name, const QString &value);

    // Hiển thị
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
    QSortFilterProxyModel *proxy_ = nullptr;
    QTimer searchDebounce_;
    bool pendingAutoApply_ = false; ///< Auto Apply bị hoãn vì offsets lệch phiên bản, chờ bản mới.

    QLabel *statusLabel_ = nullptr;
    QLabel *offsetsLabel_ = nullptr;
    QLabel *versionLabel_ = nullptr;
    QLabel *countLabel_ = nullptr;
    QLineEdit *search_ = nullptr;
    QTableView *table_ = nullptr;
    QCheckBox *autoApplyCheck_ = nullptr;
    QPushButton *addButton_ = nullptr;
    QPushButton *removeButton_ = nullptr;
    QPushButton *exportButton_ = nullptr;
    QPushButton *applyButton_ = nullptr;
};

} // namespace SimpleInjector::Ui
