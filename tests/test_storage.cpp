#include "core/app_storage.h"
#include "core/offsets_cache.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

using namespace SimpleInjector;

class StorageTests final : public QObject {
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

        QVERIFY(QFile::exists(paths.flags));           // đã chép từ bản cũ
        QFile settings(paths.settings);
        QVERIFY(settings.open(QIODevice::ReadOnly));
        QCOMPARE(settings.readAll(), QByteArray("{\"auto_apply\":false}")); // không bị ghi đè
    }

    void offsetsCacheRoundTrip()
    {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        const QString path = root.filePath(QStringLiteral("offsets.json"));

        const OffsetMap expected{{QStringLiteral("Alpha"), 0x10}, {QStringLiteral("Beta"), 0xABCDEF}};
        QString error;
        QVERIFY2(saveOffsetsCache(path, expected, QString(), &error), qPrintable(error));
        QCOMPARE(loadOffsetsCache(path), expected);
    }

    void offsetsCacheStoresVersionAlongsideOffsets()
    {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        const QString path = root.filePath(QStringLiteral("offsets.json"));

        const OffsetMap expected{{QStringLiteral("Alpha"), 0x10}};
        QString error;
        QVERIFY2(saveOffsetsCache(path, expected, QStringLiteral("version-abc123"), &error),
                 qPrintable(error));

        QString version;
        QCOMPARE(loadOffsetsCache(path, &version), expected); // __meta không lọt vào offsets
        QCOMPARE(version, QStringLiteral("version-abc123"));
    }

    void offsetsCacheWithoutVersionGivesEmptyVersion()
    {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        const QString path = root.filePath(QStringLiteral("offsets.json"));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("{\"Alpha\":\"10\"}"); // định dạng cũ, chưa có __meta
        file.close();

        QString version = QStringLiteral("stale");
        QCOMPARE(loadOffsetsCache(path, &version).size(), 1);
        QVERIFY(version.isEmpty());
    }

    void offsetsCacheSkipsCorruptEntries()
    {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        const QString path = root.filePath(QStringLiteral("offsets.json"));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("{\"Good\":\"1f\",\"Zero\":\"0\",\"Bad\":\"zz\",\"Number\":5}");
        file.close();

        const OffsetMap loaded = loadOffsetsCache(path);
        QCOMPARE(loaded.size(), 1);
        QCOMPARE(loaded.value(QStringLiteral("Good")), quintptr(0x1f));
    }
};

QTEST_GUILESS_MAIN(StorageTests)
#include "test_storage.moc"
