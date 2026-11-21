#include "core/offsets/offset_lookup.h"
#include "core/offsets/offsets_header_parser.h"

#include <QtTest>

using namespace SimpleInjector;

class OffsetsParsingTests final : public QObject {
    Q_OBJECT

private slots:
    void parseOffsetsHeader()
    {
        const QString source = QStringLiteral(
            "inline constexpr uintptr_t FFlagAlpha = 0x10;\n"
            "inline constexpr uintptr_t DFIntBeta = 0x2A;\n"
            "inline constexpr uintptr_t Pointer = 0x100;\n"
            "inline constexpr uintptr_t FFlagZero = 0x0;\n"
            "inline constexpr uintptr_t FFlagInvalid = 0xGG;\n");

        const auto offsets = SimpleInjector::parseOffsetsHeader(source);
        // Every prefixed name is stored both verbatim and without its prefix.
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
        // Real names from Roblox's offsets table: no prefix, starting with a lowercase letter.
        const QString source = QStringLiteral(
            "inline constexpr std::uintptr_t kCageHoleNum = 0x7f00;\n"
            "inline constexpr std::uintptr_t joinGameAttemptEventThrottleHundrethsPercent = 0x7f10;\n");
        const auto offsets = SimpleInjector::parseOffsetsHeader(source);
        QCOMPARE(offsets.size(), 2);
        QCOMPARE(offsets.value(QStringLiteral("kCageHoleNum")), quintptr(0x7f00));
    }

    void parseOffsetsHeaderPrefersVerbatimNameOnCollision()
    {
        // "FFlagFoo" (a real, verbatim name) and "Foo" (a different name) both exist:
        // the prefix-less variant of "FFlagFoo" must not overwrite "Foo".
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

        QCOMPARE(find(QStringLiteral("TaskSchedulerTargetFps")).value_or(0), quintptr(0x10));       // bare name
        QCOMPARE(find(QStringLiteral("DFIntTaskSchedulerTargetFps")).value_or(0), quintptr(0x10));  // with a type prefix
        QCOMPARE(find(QStringLiteral("  FIntTaskSchedulerTargetFps ")).value_or(0), quintptr(0x10)); // surrounding whitespace
        QCOMPARE(find(QStringLiteral("FIntkCageHoleNum")).value_or(0), quintptr(0x20));             // a real name that starts lowercase
        QCOMPARE(find(QStringLiteral("FFlagMigrateCameraFovDatamodel")).value_or(0), quintptr(0x30)); // verbatim
        QCOMPARE(find(QStringLiteral("MigrateCameraFovDatamodel")).value_or(0), quintptr(0x30));    // prefix-less variant
        QCOMPARE(find(QStringLiteral("FIntensity")).value_or(0), quintptr(0x40));                   // looks prefixed but matches verbatim
        QVERIFY(!find(QStringLiteral("NoSuchFlag")).has_value());
        QVERIFY(!find(QStringLiteral("FFlagNoSuchFlag")).has_value());
        QVERIFY(!find(QString()).has_value());
    }

};

QTEST_GUILESS_MAIN(OffsetsParsingTests)
#include "test_offsets_parsing.moc"
