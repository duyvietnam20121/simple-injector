#include "core/flags/flag_value.h"

#include <QJsonArray>
#include <QJsonObject>

#include <limits>
#include <QtTest>

using namespace SimpleInjector;

class FlagValueTests final : public QObject {
    Q_OBJECT

private slots:
    void jsonValueToString_data()
    {
        QTest::addColumn<QJsonValue>("input");
        QTest::addColumn<QString>("expected");
        QTest::addColumn<bool>("valid");

        QTest::newRow("string") << QJsonValue(QStringLiteral("hello")) << QStringLiteral("hello") << true;
        QTest::newRow("true") << QJsonValue(true) << QStringLiteral("true") << true;
        QTest::newRow("false") << QJsonValue(false) << QStringLiteral("false") << true;
        QTest::newRow("integer") << QJsonValue(123.0) << QStringLiteral("123") << true;
        QTest::newRow("null") << QJsonValue(QJsonValue::Null) << QString() << false;
        QJsonArray array;
        array.append(1);
        array.append(2);
        QTest::newRow("array") << QJsonValue(array) << QString() << false;
        QJsonObject object;
        object.insert(QStringLiteral("x"), 1);
        QTest::newRow("object") << QJsonValue(object) << QString() << false;
    }

    void jsonValueToString()
    {
        QFETCH(QJsonValue, input);
        QFETCH(QString, expected);
        QFETCH(bool, valid);

        bool ok = false;
        QCOMPARE(SimpleInjector::jsonValueToString(input, &ok), expected);
        QCOMPARE(ok, valid);
    }

    void parseInt32Value_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<bool>("valid");
        QTest::addColumn<qint32>("expected");

        QTest::newRow("true") << QStringLiteral("true") << true << qint32(1);
        QTest::newRow("false") << QStringLiteral("FALSE") << true << qint32(0);
        QTest::newRow("decimal") << QStringLiteral(" 123 ") << true << qint32(123);
        QTest::newRow("negative") << QStringLiteral("-42") << true << qint32(-42);
        QTest::newRow("hex") << QStringLiteral("0x7FFFFFFF") << true << std::numeric_limits<qint32>::max();
        QTest::newRow("minimum") << QStringLiteral("-2147483648") << true << std::numeric_limits<qint32>::min();
        QTest::newRow("above max") << QStringLiteral("2147483648") << false << qint32(0);
        QTest::newRow("below min") << QStringLiteral("-2147483649") << false << qint32(0);
        QTest::newRow("bad text") << QStringLiteral("12abc") << false << qint32(0);
        QTest::newRow("empty") << QString() << false << qint32(0);
    }

    void parseInt32Value()
    {
        QFETCH(QString, input);
        QFETCH(bool, valid);
        QFETCH(qint32, expected);

        qint32 actual = 987;
        const bool parsed = SimpleInjector::parseInt32Value(input, &actual);
        QCOMPARE(parsed, valid);
        if (valid)
            QCOMPARE(actual, expected);
    }

    void parseInt32ValueRejectsNullOutput()
    {
        QVERIFY(!SimpleInjector::parseInt32Value(QStringLiteral("1"), nullptr));
    }

};

QTEST_GUILESS_MAIN(FlagValueTests)
#include "test_flag_value.moc"
