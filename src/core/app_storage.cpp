#include "app_storage.h"

#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QStringList>

namespace SimpleInjector {

StoragePaths prepareStorage(const QString &rootOverride)
{
    QString root = rootOverride;
    if (root.isEmpty())
        root = qEnvironmentVariable("LOCALAPPDATA");
    if (root.isEmpty())
        root = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);

    const QDir rootDir(root);
    StoragePaths paths;
    paths.directory = rootDir.filePath(QStringLiteral("SimpleInjector"));
    QDir().mkpath(paths.directory);

    const QDir dataDir(paths.directory);
    paths.flags = dataDir.filePath(QStringLiteral("ffs.json"));
    paths.settings = dataDir.filePath(QStringLiteral("settings.json"));
    paths.offsets = dataDir.filePath(QStringLiteral("offsets.json"));

    // Giữ lại dữ liệu của bản "EZ" trước đây.
    const QDir legacyDir(rootDir.filePath(QStringLiteral("ez")));
    for (const QString &fileName : {QStringLiteral("ffs.json"),
                                    QStringLiteral("settings.json"),
                                    QStringLiteral("offsets.json")}) {
        const QString newPath = dataDir.filePath(fileName);
        const QString oldPath = legacyDir.filePath(fileName);
        if (!QFile::exists(newPath) && QFile::exists(oldPath))
            QFile::copy(oldPath, newPath);
    }
    return paths;
}

} // namespace SimpleInjector
