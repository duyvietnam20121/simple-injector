#include "core/flags/flag_name.h"

#include <QtTest>

using namespace SimpleInjector;

class FlagNameTests final : public QObject {
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

};

QTEST_GUILESS_MAIN(FlagNameTests)
#include "test_flag_name.moc"
