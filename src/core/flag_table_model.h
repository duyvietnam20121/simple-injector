#pragma once

#include <QAbstractTableModel>
#include <QStringList>

namespace SimpleInjector {

class FlagStore;

/// Mô hình bảng hai cột (Tên, Giá trị) trên FlagStore. Cột giá trị sửa được.
/// Kết hợp QSortFilterProxyModel để sắp xếp và lọc nhanh khi có hàng nghìn flag.
///
/// Model không sở hữu store. Khi store bị thay đổi từ bên ngoài (thêm/xóa),
/// gọi reload(); khi người dùng sửa giá trị trong view, model tự cập nhật store
/// rồi phát valueEdited() để nơi gọi lưu xuống đĩa.
class FlagTableModel final : public QAbstractTableModel {
    Q_OBJECT

public:
    enum Column { NameColumn = 0, ValueColumn = 1, ColumnCount = 2 };

    explicit FlagTableModel(FlagStore *store, QObject *parent = nullptr);

    /// Dựng lại toàn bộ từ store (reset model, mất vùng chọn).
    void reload();

    /// Tên flag ở hàng nguồn `row`; rỗng nếu ngoài phạm vi.
    QString nameAt(int row) const;

    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;

signals:
    /// Phát sau khi một giá trị thực sự đã đổi trong store.
    void valueEdited(const QString &name, const QString &value);

private:
    FlagStore *store_ = nullptr;
    QStringList names_; // đã sắp xếp theo tên (thứ tự khóa của QJsonObject)
};

} // namespace SimpleInjector
