#include "flag_table_model.h"

#include "flag_store.h"
#include "flag_utils.h"

#include <QColor>

namespace SimpleInjector {

namespace {

/// Thứ tự sắp xếp tăng dần: vấn đề nặng nhất lên đầu, "chưa biết" xuống cuối.
int sortRank(FlagStatus status)
{
    switch (status) {
    case FlagStatus::WriteFailed:    return 0;
    case FlagStatus::VerifyFailed:   return 1;
    case FlagStatus::MissingOffset:  return 2;
    case FlagStatus::InvalidValue:   return 3;
    case FlagStatus::Ready:          return 4;
    case FlagStatus::Applied:        return 5;
    case FlagStatus::Unknown:        return 6;
    }
    return 6;
}

QColor statusColor(FlagStatus status)
{
    switch (status) {
    case FlagStatus::Applied:       return QColor(0x61, 0xD7, 0x86); // xanh lá
    case FlagStatus::Ready:         return QColor(0xA8, 0xA8, 0xAE); // xám sáng
    case FlagStatus::VerifyFailed:  return QColor(0xFF, 0xB3, 0x47); // cam
    case FlagStatus::MissingOffset:
    case FlagStatus::InvalidValue:
    case FlagStatus::WriteFailed:   return QColor(0xFF, 0x66, 0x66); // đỏ
    case FlagStatus::Unknown:       return QColor(0x66, 0x66, 0x6B); // xám tối
    }
    return {};
}

} // namespace

FlagTableModel::FlagTableModel(FlagStore *store, QObject *parent)
    : QAbstractTableModel(parent), store_(store)
{
    names_ = store_->entries().keys();
}

QString FlagTableModel::statusText(FlagStatus status)
{
    switch (status) {
    case FlagStatus::Unknown:       return QStringLiteral("— chưa rõ");
    case FlagStatus::Ready:         return QStringLiteral("● Sẵn sàng");
    case FlagStatus::Applied:       return QStringLiteral("✓ Đã áp dụng");
    case FlagStatus::VerifyFailed:  return QStringLiteral("⚠ Không khớp khi đọc lại");
    case FlagStatus::MissingOffset: return QStringLiteral("✗ Không có offset");
    case FlagStatus::InvalidValue:  return QStringLiteral("✗ Giá trị không hỗ trợ");
    case FlagStatus::WriteFailed:   return QStringLiteral("✗ Lỗi ghi");
    }
    return {};
}

void FlagTableModel::setOffsets(const OffsetMap *offsets)
{
    offsets_ = offsets;
    emitStatusChanged();
}

void FlagTableModel::refreshStatuses()
{
    emitStatusChanged();
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
    if (applied != applyResults_.cend())
        return applied.value();

    static const OffsetMap kNoOffsets;
    return previewFlag(name, jsonValueToString(store_->entries().value(name)),
                       offsets_ ? *offsets_ : kNoOffsets);
}

QString FlagTableModel::tooltipFor(const QString &name) const
{
    const FlagResult result = resultFor(name);
    if (result.status != FlagStatus::MissingOffset || !offsets_ || offsets_->isEmpty())
        return result.detail;

    const QStringList hints = suggestOffsetNames(*offsets_, name);
    if (hints.isEmpty())
        return result.detail;
    return result.detail + QStringLiteral("\nGợi ý gần giống trong bảng offsets: ") + hints.join(QStringLiteral(", "));
}

FlagStatus FlagTableModel::statusAt(int row) const
{
    return (row >= 0 && row < names_.size()) ? resultFor(names_.at(row)).status : FlagStatus::Unknown;
}

int FlagTableModel::problemCount() const
{
    int count = 0;
    for (const QString &name : names_) {
        if (isProblem(resultFor(name).status))
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
        const FlagResult result = resultFor(name);
        switch (role) {
        case Qt::DisplayRole:    return statusText(result.status);
        case Qt::ToolTipRole:    return tooltipFor(name);
        case Qt::ForegroundRole: return statusColor(result.status);
        case StatusRole:         return static_cast<int>(result.status);
        case SortRole:           return sortRank(result.status);
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
        if (index.column() == NameColumn)
            return tooltipFor(name);
        return QStringLiteral("Nhấp đúp hoặc F2 để sửa giá trị");
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
        return false; // không cho xóa trắng giá trị

    const QString &name = names_.at(index.row());
    if (jsonValueToString(store_->entries().value(name)) == text)
        return true; // không đổi gì

    if (!store_->setValue(name, text))
        return false;

    // Kết quả Apply cũ không còn đúng với giá trị mới: quay về trạng thái dự đoán.
    applyResults_.remove(name);

    emit dataChanged(index.siblingAtColumn(NameColumn), index.siblingAtColumn(StatusColumn),
                     {Qt::DisplayRole, Qt::EditRole, Qt::ToolTipRole, Qt::ForegroundRole,
                      StatusRole, SortRole});
    emit valueEdited(name, text);
    return true;
}

// ---- FlagFilterProxyModel ---------------------------------------------------

FlagFilterProxyModel::FlagFilterProxyModel(QObject *parent) : QSortFilterProxyModel(parent)
{
    setFilterKeyColumn(-1); // lọc trên mọi cột (tên, giá trị, chữ trạng thái)
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setSortRole(FlagTableModel::SortRole);
    setDynamicSortFilter(true);
}

void FlagFilterProxyModel::setProblemsOnly(bool enabled)
{
    if (problemsOnly_ == enabled)
        return;
    problemsOnly_ = enabled;
    invalidateFilter();
}

bool FlagFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if (problemsOnly_) {
        const auto *model = qobject_cast<const FlagTableModel *>(sourceModel());
        if (model && !isProblem(model->statusAt(sourceRow)))
            return false;
    }
    return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
}

} // namespace SimpleInjector
