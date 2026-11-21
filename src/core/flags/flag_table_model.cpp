#include "core/flags/flag_table_model.h"

#include "core/flags/flag_store.h"
#include "core/flags/flag_value.h"

#include <QColor>

#include <iterator>

namespace SimpleInjector {

namespace {

/// Ascending sort order: failed first, then pending, finally applied.
int sortRank(FlagStatus status)
{
    if (status == FlagStatus::Applied)
        return 2;
    return status == FlagStatus::Pending ? 1 : 0;
}

QColor statusColor(FlagStatus status)
{
    if (status == FlagStatus::Applied)
        return QColor(0x61, 0xD7, 0x86); // green
    return status == FlagStatus::Pending ? QColor(0x8A, 0x8A, 0x90)  // gray
                                         : QColor(0xFF, 0x66, 0x66); // red
}

} // namespace

FlagTableModel::FlagTableModel(FlagStore *store, QObject *parent)
    : QAbstractTableModel(parent), store_(store)
{
    names_ = store_->entries().keys();
}

QString FlagTableModel::statusText(FlagStatus status)
{
    if (status == FlagStatus::Applied)
        return QStringLiteral("✓ Applied");
    return status == FlagStatus::Pending ? QStringLiteral("— Pending") : QStringLiteral("✗ Failed");
}

void FlagTableModel::setApplyResults(const QHash<QString, FlagResult> &results)
{
    applyResults_ = results;
    emitStatusChanged();
}

void FlagTableModel::clearApplyResults()
{
    if (applyResults_.isEmpty())
        return;
    applyResults_.clear();
    emitStatusChanged();
}

void FlagTableModel::reload()
{
    beginResetModel();
    names_ = store_->entries().keys();
    for (auto it = applyResults_.begin(); it != applyResults_.end();) {
        it = store_->entries().contains(it.key()) ? std::next(it) : applyResults_.erase(it);
    }
    endResetModel();
}

QString FlagTableModel::nameAt(int row) const
{
    return (row >= 0 && row < names_.size()) ? names_.at(row) : QString();
}

FlagResult FlagTableModel::resultFor(const QString &name) const
{
    const auto applied = applyResults_.constFind(name);
    return applied != applyResults_.cend() ? applied.value() : FlagResult{};
}

FlagStatus FlagTableModel::statusAt(int row) const
{
    return (row >= 0 && row < names_.size()) ? resultFor(names_.at(row)).status : FlagStatus::Pending;
}

int FlagTableModel::failedCount() const
{
    int count = 0;
    for (const QString &name : names_) {
        if (isFailed(resultFor(name).status))
            ++count;
    }
    return count;
}

void FlagTableModel::emitStatusChanged()
{
    if (names_.isEmpty())
        return;
    emit dataChanged(index(0, StatusColumn), index(static_cast<int>(names_.size()) - 1, StatusColumn));
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

    if (index.column() == StatusColumn) {
        const FlagStatus status = resultFor(name).status;
        switch (role) {
        case Qt::DisplayRole:    return statusText(status);
        case Qt::ForegroundRole: return statusColor(status);
        case StatusRole:         return static_cast<int>(status);
        case SortRole:           return sortRank(status);
        default:                 return {};
        }
    }

    const QString value = jsonValueToString(store_->entries().value(name));
    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
    case SortRole:
        return index.column() == NameColumn ? name : value;
    case Qt::ToolTipRole:
        return index.column() == ValueColumn ? QStringLiteral("Double-click or press F2 to edit the value")
                                             : QVariant();
    default:
        return {};
    }
}

QVariant FlagTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};
    switch (section) {
    case NameColumn:   return QStringLiteral("NAME");
    case ValueColumn:  return QStringLiteral("VALUE");
    case StatusColumn: return QStringLiteral("STATUS");
    default:           return {};
    }
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
        return false; // a value cannot be blanked out

    const QString &name = names_.at(index.row());
    if (jsonValueToString(store_->entries().value(name)) == text)
        return true; // nothing changed

    if (!store_->setValue(name, text))
        return false;

    // The old apply result no longer matches the new value: go back to "pending".
    applyResults_.remove(name);

    emit dataChanged(index.siblingAtColumn(NameColumn), index.siblingAtColumn(StatusColumn),
                     {Qt::DisplayRole, Qt::EditRole, Qt::ToolTipRole, Qt::ForegroundRole,
                      StatusRole, SortRole});
    emit valueEdited(name, text);
    return true;
}

} // namespace SimpleInjector
