#pragma once

#include "core/apply/flag_status.h"

#include <QAbstractTableModel>
#include <QHash>
#include <QStringList>

namespace SimpleInjector {

class FlagStore;

/// Three-column table model (Name, Value, Status) over a FlagStore. The Value column is editable.
///
/// The Status column has only three easy-to-read states for each flag:
///   "— Pending" (not applied yet, or the value was just edited), "✓ Applied", "✗ Failed".
/// The detailed failure reasons (missing offset, bad value type, read-back mismatch, ...) stay
/// in the ApplyReport and are exposed through StatusRole, but are not shown in the table.
///
/// The model does not own the store. When the store is changed from outside (add/remove) call
/// reload(). When the user edits a value, the model updates the store, discards that flag's old
/// apply result (it no longer matches the new value) and emits valueEdited() so the caller can
/// persist the change.
class FlagTableModel final : public QAbstractTableModel {
    Q_OBJECT

public:
    enum Column { NameColumn = 0, ValueColumn = 1, StatusColumn = 2, ColumnCount = 3 };

    enum Role {
        SortRole = Qt::UserRole,       ///< Value used for sorting (status: failed first).
        StatusRole = Qt::UserRole + 1, ///< The FlagStatus as an int.
    };

    explicit FlagTableModel(FlagStore *store, QObject *parent = nullptr);

    /// Records the outcome of an Apply run (replaces any previous results).
    void setApplyResults(const QHash<QString, FlagResult> &results);
    void clearApplyResults();

    /// Rebuilds everything from the store (resets the model, losing the selection).
    /// Keeps the apply results of flags that still exist.
    void reload();

    QString nameAt(int row) const;
    FlagStatus statusAt(int row) const;
    /// Number of flags whose last Apply failed (see isFailed()).
    int failedCount() const;

    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;

    /// Short display text for a status (with a symbol).
    static QString statusText(FlagStatus status);

signals:
    /// Emitted after a value has actually changed in the store.
    void valueEdited(const QString &name, const QString &value);

private:
    FlagResult resultFor(const QString &name) const;
    void emitStatusChanged();

    FlagStore *store_ = nullptr;
    QStringList names_; // sorted by name (the key order of QJsonObject)
    QHash<QString, FlagResult> applyResults_;
};

} // namespace SimpleInjector
