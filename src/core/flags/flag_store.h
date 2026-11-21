#pragma once

#include <QJsonObject>
#include <QString>
#include <QStringList>

namespace SimpleInjector {

/// Persistent collection of the flags the user added: prefix-less name -> value as a string.
///
/// Depends on Qt Core only, so it is unit-testable without any UI.
class FlagStore {
public:
    struct MergeResult {
        int added = 0;   ///< Entries added or updated.
        int skipped = 0; ///< Entries ignored (empty name or unsupported value type).
    };

    explicit FlagStore(QString path = {});

    void setPath(QString path) { path_ = std::move(path); }
    const QString &path() const { return path_; }

    /// Loads from disk; a missing or corrupt file yields an empty store.
    void load();
    bool save(QString *error = nullptr) const;

    /// Merges an external JSON object into the store (existing names are overwritten).
    MergeResult merge(const QJsonObject &incoming);

    /// Sets the value of an EXISTING entry; false if the name is not in the store.
    bool setValue(const QString &name, const QString &value);

    /// Removes entries by name and returns how many were actually removed.
    int remove(const QStringList &names);

    /// Extracts a subset by name (unknown names are ignored).
    QJsonObject subset(const QStringList &names) const;

    const QJsonObject &entries() const { return entries_; }
    int size() const { return entries_.size(); }
    bool isEmpty() const { return entries_.isEmpty(); }

private:
    QString path_;
    QJsonObject entries_;
};

} // namespace SimpleInjector
