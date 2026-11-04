#include "core/flag_store.h"

#include <QJsonArray>
#include <QTemporaryDir>
#include <QtTest>

using SimpleInjector::FlagStore;

class FlagStoreTests final : public QObject {
    Q_OBJECT

private slots:
    void mergeNormalizesNamesAndValues()
    {
        FlagStore store;
        QJsonObject incoming;
        incoming.insert(QStringLiteral("FFlagAlpha"), true);
        incoming.insert(QStringLiteral("DFIntBeta"), 120);
        incoming.insert(QStringLiteral("FStringGamma"), QStringLiteral("text"));

        const auto result = store.merge(incoming);
        QCOMPARE(result.added, 3);
        QCOMPARE(result.skipped, 0);
        QCOMPARE(store.entries().value(QStringLiteral("Alpha")).toString(), QStringLiteral("true"));
        QCOMPARE(store.entries().value(QStringLiteral("Beta")).toString(), QStringLiteral("120"));
        QCOMPARE(store.entries().value(QStringLiteral("Gamma")).toString(), QStringLiteral("text"));
    }

    void mergeSkipsUnsupportedAndEmptyNames()
    {
        FlagStore store;
        QJsonObject incoming;
        incoming.insert(QStringLiteral("Good"), 1);
        incoming.insert(QStringLiteral("NullValue"), QJsonValue(QJsonValue::Null));
        incoming.insert(QStringLiteral("ArrayValue"), QJsonArray{1, 2});
        incoming.insert(QStringLiteral("FFlag"), true); // tên rỗng sau khi bỏ tiền tố

        const auto result = store.merge(incoming);
        QCOMPARE(result.added, 1);
        QCOMPARE(result.skipped, 3);
        QCOMPARE(store.size(), 1);
    }

    void mergeOverwritesExistingEntry()
    {
        FlagStore store;
        QJsonObject first;
        first.insert(QStringLiteral("FFlagX"), false);
        store.merge(first);

        QJsonObject second;
        second.insert(QStringLiteral("DFFlagX"), true);
        store.merge(second);

        QCOMPARE(store.size(), 1);
        QCOMPARE(store.entries().value(QStringLiteral("X")).toString(), QStringLiteral("true"));
    }

    void removeCountsOnlyExistingNames()
    {
        FlagStore store;
        QJsonObject incoming;
        incoming.insert(QStringLiteral("A"), 1);
        incoming.insert(QStringLiteral("B"), 2);
        store.merge(incoming);

        QCOMPARE(store.remove({QStringLiteral("A"), QStringLiteral("Missing")}), 1);
        QCOMPARE(store.size(), 1);
        QVERIFY(store.entries().contains(QStringLiteral("B")));
    }

    void subsetIgnoresUnknownNames()
    {
        FlagStore store;
        QJsonObject incoming;
        incoming.insert(QStringLiteral("A"), 1);
        incoming.insert(QStringLiteral("B"), 2);
        store.merge(incoming);

        const QJsonObject subset = store.subset({QStringLiteral("B"), QStringLiteral("Nope")});
        QCOMPARE(subset.size(), 1);
        QVERIFY(subset.contains(QStringLiteral("B")));
    }

    void saveAndLoadRoundTrip()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = directory.filePath(QStringLiteral("ffs.json"));

        FlagStore writer(path);
        QJsonObject incoming;
        incoming.insert(QStringLiteral("FFlagKeep"), true);
        writer.merge(incoming);
        QString error;
        QVERIFY2(writer.save(&error), qPrintable(error));

        FlagStore reader(path);
        reader.load();
        QCOMPARE(reader.entries(), writer.entries());
    }

    void loadMissingFileGivesEmptyStore()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        FlagStore store(directory.filePath(QStringLiteral("absent.json")));
        store.load();
        QVERIFY(store.isEmpty());
    }
};

QTEST_GUILESS_MAIN(FlagStoreTests)
#include "test_flag_store.moc"
