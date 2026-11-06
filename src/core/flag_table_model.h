#pragma once

#include "flag_applier.h"

#include <QAbstractTableModel>
#include <QHash>
#include <QSortFilterProxyModel>
#include <QStringList>

namespace SimpleInjector {

class FlagStore;

/// Mô hình bảng ba cột (Tên, Giá trị, Trạng thái) trên FlagStore. Cột giá trị sửa được.
///
/// Cột Trạng thái cho biết TỪNG flag: chưa biết / sẵn sàng / đã áp dụng / không có offset /
/// giá trị không hỗ trợ / ghi nhưng đọc lại không khớp / lỗi ghi. Trước khi Apply, trạng thái được
/// dự đoán từ bảng offsets (previewFlag); sau Apply, trạng thái là kết quả thực tế.
///
/// Model không sở hữu store hay bảng offsets. Khi store bị thay đổi từ bên ngoài (thêm/xóa),
/// gọi reload(); khi người dùng sửa giá trị, model tự cập nhật store, xóa kết quả Apply cũ của
/// flag đó (vì giá trị đã đổi) rồi phát valueEdited() để nơi gọi lưu xuống đĩa.
class FlagTableModel final : public QAbstractTableModel {
    Q_OBJECT

public:
    enum Column { NameColumn = 0, ValueColumn = 1, StatusColumn = 2, ColumnCount = 3 };

    enum Role {
        SortRole = Qt::UserRole,       ///< Giá trị để sắp xếp (trạng thái: số, vấn đề lên trước).
        StatusRole = Qt::UserRole + 1, ///< FlagStatus dạng int.
    };

    explicit FlagTableModel(FlagStore *store, QObject *parent = nullptr);

    /// Bảng offsets dùng để dự đoán trạng thái; con trỏ phải sống lâu hơn model (có thể null).
    void setOffsets(const OffsetMap *offsets);
    /// Báo rằng bảng offsets đã đổi nội dung: tính lại cột Trạng thái.
    void refreshStatuses();

    /// Ghi nhận kết quả của một lần Apply (thay thế kết quả cũ).
    void setApplyResults(const QHash<QString, FlagResult> &results);
    void clearApplyResults();

    /// Dựng lại toàn bộ từ store (reset model, mất vùng chọn). Giữ kết quả Apply của flag còn tồn tại.
    void reload();

    QString nameAt(int row) const;
    FlagStatus statusAt(int row) const;
    /// Số flag đang ở trạng thái "có vấn đề" (xem isProblem()).
    int problemCount() const;

    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;

    /// Chữ hiển thị ngắn cho một trạng thái (kèm ký hiệu).
    static QString statusText(FlagStatus status);

signals:
    /// Phát sau khi một giá trị thực sự đã đổi trong store.
    void valueEdited(const QString &name, const QString &value);

private:
    FlagResult resultFor(const QString &name) const;
    QString tooltipFor(const QString &name) const; ///< Chi tiết + gợi ý tên gần giống nếu thiếu offset.
    void emitStatusChanged();

    FlagStore *store_ = nullptr;
    const OffsetMap *offsets_ = nullptr;
    QStringList names_; // đã sắp xếp theo tên (thứ tự khóa của QJsonObject)
    QHash<QString, FlagResult> applyResults_;
};

/// Proxy sắp xếp + lọc: lọc văn bản trên mọi cột (kể cả chữ trạng thái) và tùy chọn
/// "chỉ hiện flag có vấn đề".
class FlagFilterProxyModel final : public QSortFilterProxyModel {
    Q_OBJECT

public:
    explicit FlagFilterProxyModel(QObject *parent = nullptr);

    void setProblemsOnly(bool enabled);
    bool problemsOnly() const { return problemsOnly_; }

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    bool problemsOnly_ = false;
};

} // namespace SimpleInjector
