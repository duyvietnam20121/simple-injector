#include "flag_table_model.h"

#include "flag_store.h"
#include "flag_utils.h"

namespace SimpleInjector {

FlagTableModel::FlagTableModel(FlagStore *store, QObject *parent)
    : QAbstractTableModel(parent), store_(store)
{
    names_ = store_->entries().keys();
}

void FlagTableModel::reload()
{
    beginResetModel();
    names_ = store_->entries().keys();
    endResetModel();
}

QString FlagTableModel::nameAt(int row) const
{
    return (row >= 0 && row < names_.size()) ? names_.at(row) : QString();
}

int FlagTableModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(names_.size());
}

int FlagTableModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant FlagTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= names_.size())
        return {};

    const QString &name = names_.at(index.row());
    const QString value = jsonValueToString(store_->entries().value(name));

    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
        return index.column() == NameColumn ? name : value;
    case Qt::ToolTipRole: {
        qint32 ignored = 0;
        return parseInt32Value(value, &ignored)
            ? QStringLiteral("Có thể Apply vào Roblox (bool / int32)")
            : QStringLiteral("Chỉ lưu và xuất; không thể Apply (không phải bool / int32)");
    }
    default:
        return {};
    }
}

QVariant FlagTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};
    return section == NameColumn ? QStringLiteral("NAME")
         : section == ValueColumn ? QStringLiteral("VALUE")
         : QVariant();
}

Qt::ItemFlags FlagTableModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags result = QAbstractTableModel::flags(index);
    if (index.isValid() && index.column() == ValueColumn)
        result |= Qt::ItemIsEditable;
    return result;
}

bool FlagTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole || !index.isValid() || index.column() != ValueColumn
        || index.row() >= names_.size())
        return false;

    const QString text = value.toString().trimmed();
    if (text.isEmpty())
        return false; // không cho xóa trắng giá trị

    const QString &name = names_.at(index.row());
    if (jsonValueToString(store_->entries().value(name)) == text)
        return true; // không đổi gì

    if (!store_->setValue(name, text))
        return false;

    emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole, Qt::ToolTipRole});
    emit valueEdited(name, text);
    return true;
}

} // namespace SimpleInjector
