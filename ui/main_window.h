#pragma once

#include "core/app_storage.h"
#include "core/flag_store.h"
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
class QTableWidget;

namespace SimpleInjector {
class ProcessMonitor;
}

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
    void applyAll();

    // Phản hồi từ service
    void onProcessStatusChanged(const RobloxProcess::Status &status);
    void onOffsetsFinished(OffsetsService::Outcome outcome, const QString &detail);

    // Hiển thị
    void refreshTable();
    void updateOffsetsLabel();
    void updateControls();
    void setStatus(const QString &message, bool good);
    QStringList selectedNames() const;
    void saveSettings();

    StoragePaths paths_;
    FlagStore flags_;
    QJsonObject settings_;
    OffsetsService *offsets_ = nullptr;
    ProcessMonitor *monitor_ = nullptr;
    QTimer searchDebounce_;

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
};

} // namespace SimpleInjector::Ui
