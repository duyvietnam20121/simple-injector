#include "flag_store.h"

#include "flag_utils.h"

namespace SimpleInjector {

FlagStore::FlagStore(QString path) : path_(std::move(path)) {}

void FlagStore::load()
{
    entries_ = readJsonObject(path_, {});
}

bool FlagStore::save(QString *error) const
{
    return writeJsonAtomically(path_, entries_, error);
}

FlagStore::MergeResult FlagStore::merge(const QJsonObject &incoming)
{
    MergeResult result;
    for (auto it = incoming.begin(); it != incoming.end(); ++it) {
        bool valid = false;
        const QString value = jsonValueToString(it.value(), &valid);
        const QString name = cleanPrefix(it.key());

        if (!valid || name.isEmpty()) {
            ++result.skipped;
            continue;
        }
        entries_.insert(name, value);
        ++result.added;
    }
    return result;
}

int FlagStore::remove(const QStringList &names)
{
    int removed = 0;
    for (const QString &name : names) {
        if (entries_.contains(name)) {
            entries_.remove(name);
            ++removed;
        }
    }
    return removed;
}

QJsonObject FlagStore::subset(const QStringList &names) const
{
    QJsonObject result;
    for (const QString &name : names) {
        if (entries_.contains(name))
            result.insert(name, entries_.value(name));
    }
    return result;
}

} // namespace SimpleInjector
