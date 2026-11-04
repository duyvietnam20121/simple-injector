#include "core/roblox_version.h"

#include <QtTest>

using namespace SimpleInjector;

class RobloxVersionTests final : public QObject {
    Q_OBJECT

private slots:
    void parsesDeclaredClientVersion()
    {
        const QString source = QStringLiteral(
            "// Generated | version-1111111111111111 | 2026\n"
            "inline constexpr std::string_view ClientVersion = \"version-5a21b87bbc2641cf\";\n");
        QCOMPARE(parseOffsetsVersion(source), QStringLiteral("version-5a21b87bbc2641cf"));
    }

    void fallsBackToHeaderComment()
    {
        const QString source = QStringLiteral("// Generated | version-5a21b87bbc2641cf | date\nint x;");
        QCOMPARE(parseOffsetsVersion(source), QStringLiteral("version-5a21b87bbc2641cf"));
    }

    void ignoresVersionLikeTextFarFromHeader()
    {
        const QString source = QString(3000, QLatin1Char(' ')) + QStringLiteral("version-5a21b87bbc2641cf");
        QVERIFY(parseOffsetsVersion(source).isEmpty());
    }

    void returnsEmptyWhenAbsent()
    {
        QVERIFY(parseOffsetsVersion(QStringLiteral("Alpha = 0x10;")).isEmpty());
    }

    void versionFromImagePath_data()
    {
        QTest::addColumn<QString>("path");
        QTest::addColumn<QString>("expected");
        QTest::newRow("windows") << QStringLiteral("C:\\Users\\a\\AppData\\Local\\Roblox\\Versions\\version-abc123\\RobloxPlayerBeta.exe")
                                 << QStringLiteral("version-abc123");
        QTest::newRow("forward slashes") << QStringLiteral("C:/Roblox/Versions/version-abc123/RobloxPlayerBeta.exe")
                                         << QStringLiteral("version-abc123");
        QTest::newRow("case insensitive prefix") << QStringLiteral("C:\\R\\VERSION-ABC\\x.exe")
                                                 << QStringLiteral("VERSION-ABC");
        QTest::newRow("parent not a version") << QStringLiteral("C:\\Games\\Roblox\\RobloxPlayerBeta.exe") << QString();
        QTest::newRow("no directory") << QStringLiteral("RobloxPlayerBeta.exe") << QString();
        QTest::newRow("empty") << QString() << QString();
    }

    void versionFromImagePath()
    {
        QFETCH(QString, path);
        QFETCH(QString, expected);
        QCOMPARE(SimpleInjector::versionFromImagePath(path), expected);
    }

    void compareVersionsCases()
    {
        QCOMPARE(compareVersions(QStringLiteral("version-a"), QStringLiteral("version-a")), VersionMatch::Match);
        QCOMPARE(compareVersions(QStringLiteral("version-a"), QStringLiteral("VERSION-A")), VersionMatch::Match);
        QCOMPARE(compareVersions(QStringLiteral("version-a"), QStringLiteral("version-b")), VersionMatch::Mismatch);
        QCOMPARE(compareVersions(QString(), QStringLiteral("version-b")), VersionMatch::Unknown);
        QCOMPARE(compareVersions(QStringLiteral("version-a"), QString()), VersionMatch::Unknown);
        QCOMPARE(compareVersions(QString(), QString()), VersionMatch::Unknown);
    }
};

QTEST_GUILESS_MAIN(RobloxVersionTests)
#include "test_roblox_version.moc"
