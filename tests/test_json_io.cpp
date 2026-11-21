#include "core/io/json_io.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTemporaryDir>

#include <QtTest>

using namespace SimpleInjector;

class JsonIoTests final : public QObject {
    Q_OBJECT

private slots:
    void parseJsonObjectAcceptsObject()
    {
        QString error;
        const QJsonObject object = SimpleInjector::parseJsonObject(QStringLiteral("{\"a\": 1}"), &error);
        QVERIFY(error.isEmpty());
        QCOMPARE(object.value(QStringLiteral("a")).toInt(), 1);
    }

    void parseJsonObjectRejectsBadInput_data()
    {
        QTest::addColumn<QString>("text");
        QTest::newRow("syntax error") << QStringLiteral("{oops");
        QTest::newRow("array root") << QStringLiteral("[1, 2]");
        QTest::newRow("scalar root") << QStringLiteral("42");
        QTest::newRow("empty") << QString();
    }

    void parseJsonObjectRejectsBadInput()
    {
        QFETCH(QString, text);
        QString error;
        QVERIFY(SimpleInjector::parseJsonObject(text, &error).isEmpty());
        QVERIFY(!error.isEmpty());
    }

    void jsonRoundTripUsesAtomicWrite()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());

        const QString path = directory.filePath(QStringLiteral("flags.json"));
        QJsonObject expected;
        expected.insert(QStringLiteral("FeatureEnabled"), true);
        expected.insert(QStringLiteral("FrameLimit"), 120);
        expected.insert(QStringLiteral("TextValue"), QStringLiteral("hello"));

        QString error;
        QVERIFY2(SimpleInjector::writeJsonAtomically(path, expected, &error), qPrintable(error));
        QCOMPARE(SimpleInjector::readJsonObject(path), expected);
    }

    void readJsonReturnsFallbackForInvalidDocument()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());

        const QString path = directory.filePath(QStringLiteral("invalid.json"));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        file.write("[this is not an object]");
        file.close();

        QJsonObject fallback;
        fallback.insert(QStringLiteral("fallback"), true);
        QCOMPARE(SimpleInjector::readJsonObject(path, fallback), fallback);
    }

    void writeJsonFailsForMissingDirectory()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());

        QString error;
        const QString path = directory.filePath(QStringLiteral("missing/flags.json"));
        QVERIFY(!SimpleInjector::writeJsonAtomically(path, {}, &error));
        QVERIFY(!error.isEmpty());
    }
};

QTEST_GUILESS_MAIN(JsonIoTests)
#include "test_json_io.moc"
