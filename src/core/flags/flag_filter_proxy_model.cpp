#include "core/flags/flag_filter_proxy_model.h"

#include "core/flags/flag_table_model.h"

namespace SimpleInjector {

FlagFilterProxyModel::FlagFilterProxyModel(QObject *parent) : QSortFilterProxyModel(parent)
{
    setFilterKeyColumn(-1); // filter on every column (name, value, status text)
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setSortRole(FlagTableModel::SortRole);
    setDynamicSortFilter(true);
}

void FlagFilterProxyModel::setFailedOnly(bool enabled)
{
    if (failedOnly_ == enabled)
        return;
    failedOnly_ = enabled;
    invalidateFilter();
}

bool FlagFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if (failedOnly_) {
        const auto *model = qobject_cast<const FlagTableModel *>(sourceModel());
        if (model && !isFailed(model->statusAt(sourceRow)))
            return false;
    }
    return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
}

} // namespace SimpleInjector
