#include "core/io/app_storage.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include <QtTest>

using namespace SimpleInjector;

class AppStorageTests final : public QObject {
    Q_OBJECT

private slots:
    void createsDirectoryAndPaths()
    {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        const StoragePaths paths = prepareStorage(root.path());

        QVERIFY(QDir(paths.directory).exists());
        QVERIFY(paths.flags.endsWith(QStringLiteral("ffs.json")));
        QVERIFY(paths.settings.endsWith(QStringLiteral("settings.json")));
        QVERIFY(paths.offsets.endsWith(QStringLiteral("offsets.json")));
    }

    void migratesLegacyFilesWithoutOverwriting()
    {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        QVERIFY(QDir(root.path()).mkpath(QStringLiteral("ez")));
        QVERIFY(QDir(root.path()).mkpath(QStringLiteral("SimpleInjector")));

        auto writeFile = [](const QString &path, const QByteArray &data) {
            QFile file(path);
            QVERIFY(file.open(QIODevice::WriteOnly));
            file.write(data);
        };
        writeFile(root.filePath(QStringLiteral("ez/ffs.json")), "{\"old\":\"1\"}");
        writeFile(root.filePath(QStringLiteral("ez/settings.json")), "{\"auto_apply\":true}");
        writeFile(root.filePath(QStringLiteral("SimpleInjector/settings.json")), "{\"auto_apply\":false}");

        const StoragePaths paths = prepareStorage(root.path());

        QVERIFY(QFile::exists(paths.flags));           // copied from the legacy folder
        QFile settings(paths.settings);
        QVERIFY(settings.open(QIODevice::ReadOnly));
        QCOMPARE(settings.readAll(), QByteArray("{\"auto_apply\":false}")); // not overwritten
    }
};

QTEST_GUILESS_MAIN(AppStorageTests)
#include "test_app_storage.moc"
