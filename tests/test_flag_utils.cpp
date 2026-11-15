#include "core/flag_utils.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTemporaryDir>

#include <limits>
#include <QtTest>

class FlagUtilsTests final : public QObject {
    Q_OBJECT

private slots:
    void cleanPrefix_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<QString>("expected");

        QTest::newRow("flag") << QStringLiteral("FFlagExample") << QStringLiteral("Example");
        QTest::newRow("dynamic flag") << QStringLiteral("DFFlagExample") << QStringLiteral("Example");
        QTest::newRow("integer") << QStringLiteral("FIntLimit") << QStringLiteral("Limit");
        QTest::newRow("dynamic integer") << QStringLiteral("DFIntLimit") << QStringLiteral("Limit");
        QTest::newRow("string") << QStringLiteral("FStringText") << QStringLiteral("Text");
        QTest::newRow("dynamic string") << QStringLiteral("DFStringText") << QStringLiteral("Text");
        QTest::newRow("log") << QStringLiteral("FLogNetwork") << QStringLiteral("Network");
        QTest::newRow("dynamic log") << QStringLiteral("DFLogNetwork") << QStringLiteral("Network");
        QTest::newRow("synchronized flag") << QStringLiteral("SFFlagExample") << QStringLiteral("Example");
        QTest::newRow("lowercase name after prefix") << QStringLiteral("FIntkCageHoleNum") << QStringLiteral("kCageHoleNum");
        QTest::newRow("lowercase after dynamic prefix") << QStringLiteral("DFIntjoinGameAttemptEventThrottleHundrethsPercent")
                                                       << QStringLiteral("joinGameAttemptEventThrottleHundrethsPercent");
        QTest::newRow("digit after prefix") << QStringLiteral("FInt2Value") << QStringLiteral("2Value");
        QTest::newRow("only prefix") << QStringLiteral("FFlag") << QString();
        QTest::newRow("only one prefix removed") << QStringLiteral("FFlagFIntX") << QStringLiteral("FIntX");
        QTest::newRow("unprefixed") << QStringLiteral("AlreadyClean") << QStringLiteral("AlreadyClean");
        QTest::newRow("trim") << QStringLiteral("  FFlagTrimmed  ") << QStringLiteral("Trimmed");
        QTest::newRow("tab and newline whitespace")
            << QStringLiteral("\tDFIntFrameLimit\n") << QStringLiteral("FrameLimit");
    }

    void cleanPrefix()
    {
        QFETCH(QString, input);
        QFETCH(QString, expected);
        QCOMPARE(SimpleInjector::cleanPrefix(input), expected);
    }

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

    void parseOffsetsHeader()
    {
        const QString source = QStringLiteral(
            "inline constexpr uintptr_t FFlagAlpha = 0x10;\n"
            "inline constexpr uintptr_t DFIntBeta = 0x2A;\n"
            "inline constexpr uintptr_t Pointer = 0x100;\n"
            "inline constexpr uintptr_t FFlagZero = 0x0;\n"
            "inline constexpr uintptr_t FFlagInvalid = 0xGG;\n");

        const auto offsets = SimpleInjector::parseOffsetsHeader(source);
        // Mỗi tên có tiền tố được lưu cả nguyên văn lẫn bản đã gỡ tiền tố.
        QCOMPARE(offsets.size(), 5);
        QCOMPARE(offsets.value(QStringLiteral("Alpha")), quintptr(0x10));
        QCOMPARE(offsets.value(QStringLiteral("FFlagAlpha")), quintptr(0x10));
        QCOMPARE(offsets.value(QStringLiteral("Beta")), quintptr(0x2A));
        QCOMPARE(offsets.value(QStringLiteral("Pointer")), quintptr(0x100));
        QVERIFY(!offsets.contains(QStringLiteral("Zero")));
        QVERIFY(!offsets.contains(QStringLiteral("FFlagZero")));
    }

    void parseOffsetsHeaderKeepsBareLowercaseNames()
    {
        // Tên thật trong bảng offsets của Roblox: không có tiền tố, bắt đầu bằng chữ thường.
        const QString source = QStringLiteral(
            "inline constexpr std::uintptr_t kCageHoleNum = 0x7f00;\n"
            "inline constexpr std::uintptr_t joinGameAttemptEventThrottleHundrethsPercent = 0x7f10;\n");
        const auto offsets = SimpleInjector::parseOffsetsHeader(source);
        QCOMPARE(offsets.size(), 2);
        QCOMPARE(offsets.value(QStringLiteral("kCageHoleNum")), quintptr(0x7f00));
    }

    void parseOffsetsHeaderPrefersVerbatimNameOnCollision()
    {
        // "FFlagFoo" (tên thật, nguyên văn) và "Foo" (tên khác) cùng tồn tại:
        // bản đã gỡ tiền tố của "FFlagFoo" không được đè lên "Foo".
        const QString source = QStringLiteral(
            "inline constexpr std::uintptr_t FFlagFoo = 0x10;\n"
            "inline constexpr std::uintptr_t Foo = 0x20;\n");
        const auto offsets = SimpleInjector::parseOffsetsHeader(source);
        QCOMPARE(offsets.value(QStringLiteral("Foo")), quintptr(0x20));
        QCOMPARE(offsets.value(QStringLiteral("FFlagFoo")), quintptr(0x10));
    }

    void lookupOffsetFindsFlagsInEveryNamingForm()
    {
        const QHash<QString, quintptr> offsets = SimpleInjector::parseOffsetsHeader(QStringLiteral(
            "inline constexpr std::uintptr_t TaskSchedulerTargetFps = 0x10;\n"
            "inline constexpr std::uintptr_t kCageHoleNum = 0x20;\n"
            "inline constexpr std::uintptr_t FFlagMigrateCameraFovDatamodel = 0x30;\n"
            "inline constexpr std::uintptr_t FIntensity = 0x40;\n"));

        auto find = [&](const QString &name) { return SimpleInjector::lookupOffset(offsets, name); };

        QCOMPARE(find(QStringLiteral("TaskSchedulerTargetFps")).value_or(0), quintptr(0x10));       // tên trần
        QCOMPARE(find(QStringLiteral("DFIntTaskSchedulerTargetFps")).value_or(0), quintptr(0x10));  // có tiền tố kiểu
        QCOMPARE(find(QStringLiteral("  FIntTaskSchedulerTargetFps ")).value_or(0), quintptr(0x10)); // có khoảng trắng
        QCOMPARE(find(QStringLiteral("FIntkCageHoleNum")).value_or(0), quintptr(0x20));             // tên thật bắt đầu chữ thường
        QCOMPARE(find(QStringLiteral("FFlagMigrateCameraFovDatamodel")).value_or(0), quintptr(0x30)); // nguyên văn
        QCOMPARE(find(QStringLiteral("MigrateCameraFovDatamodel")).value_or(0), quintptr(0x30));    // dạng đã gỡ tiền tố
        QCOMPARE(find(QStringLiteral("FIntensity")).value_or(0), quintptr(0x40));                   // trùng dạng tiền tố nhưng khớp nguyên văn
        QVERIFY(!find(QStringLiteral("NoSuchFlag")).has_value());
        QVERIFY(!find(QStringLiteral("FFlagNoSuchFlag")).has_value());
        QVERIFY(!find(QString()).has_value());
    }

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

QTEST_GUILESS_MAIN(FlagUtilsTests)
#include "test_flag_utils.moc"
