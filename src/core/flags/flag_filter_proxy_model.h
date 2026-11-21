#pragma once

#include <QSortFilterProxyModel>

namespace SimpleInjector {

/// Sorting + filtering proxy for FlagTableModel. Text filtering applies to every column
/// (including the status text), and "failed only" hides every flag that did not fail its
/// last Apply.
class FlagFilterProxyModel final : public QSortFilterProxyModel {
    Q_OBJECT

public:
    explicit FlagFilterProxyModel(QObject *parent = nullptr);

    void setFailedOnly(bool enabled);
    bool failedOnly() const { return failedOnly_; }

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    bool failedOnly_ = false;
};

} // namespace SimpleInjector
