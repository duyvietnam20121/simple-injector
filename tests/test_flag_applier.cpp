#include "core/apply/flag_applier.h"
#include "core/offsets/offsets_header_parser.h"

#include <QMap>
#include <QSet>
#include <QtTest>

#include <limits>

using namespace SimpleInjector;

namespace {

/// Fake process memory. It can simulate: a failing write, the game overwriting a value (the
/// write reports success but the read-back differs), and unreadable memory.
class FakeMemory final : public IProcessMemory {
public:
    bool writeInt32(quintptr address, qint32 value) override
    {
        if (failingWrites.contains(address))
            return false;
        if (!ignoredWrites.contains(address))
            memory.insert(address, value);
        return true;
    }

    bool readInt32(quintptr address, qint32 *out) override
    {
        if (unreadable.contains(address) || !memory.contains(address))
            return false;
        *out = memory.value(address);
        return true;
    }

    QMap<quintptr, qint32> memory;
    QSet<quintptr> failingWrites;
    QSet<quintptr> ignoredWrites; ///< The write reports success but the value does not change.
    QSet<quintptr> unreadable;
};

} // namespace

class FlagApplierTests final : public QObject {
    Q_OBJECT

private slots:
    void writesAtImageBasePlusOffsetAndVerifies()
    {
        FakeMemory memory;
        const OffsetMap offsets{{QStringLiteral("Alpha"), 0x10}, {QStringLiteral("Beta"), 0x20}};
        QJsonObject flags;
        flags.insert(QStringLiteral("Alpha"), QStringLiteral("true"));
        flags.insert(QStringLiteral("Beta"), QStringLiteral("-7"));

        const ApplyReport report = applyFlags(flags, offsets, 0x1000, memory);
        QCOMPARE(report.applied(), 2);
        QCOMPARE(report.failed(), 0);
        QCOMPARE(report.total(), 2);
        QCOMPARE(memory.memory.value(0x1010), qint32(1));
        QCOMPARE(memory.memory.value(0x1020), qint32(-7));
    }

    void readBackMismatchIsAFailure()
    {
        FakeMemory memory;
        memory.ignoredWrites.insert(0x1000 + 0x10);
        memory.memory.insert(0x1000 + 0x10, 99); // the old value stays
        const OffsetMap offsets{{QStringLiteral("Stale"), 0x10}};
        QJsonObject flags;
        flags.insert(QStringLiteral("Stale"), QStringLiteral("1"));

        const ApplyReport report = applyFlags(flags, offsets, 0x1000, memory);
        QCOMPARE(report.applied(), 0);
        QCOMPARE(report.count(FlagStatus::VerifyFailed), 1);
        QCOMPARE(report.summary(), QStringLiteral("Applied 0/1 flags · 1 failed"));
    }

    void unreadableMemoryCountsAsVerifyFailure()
    {
        FakeMemory memory;
        memory.unreadable.insert(0x1000 + 0x10);
        const OffsetMap offsets{{QStringLiteral("Hidden"), 0x10}};
        QJsonObject flags;
        flags.insert(QStringLiteral("Hidden"), QStringLiteral("1"));

        QCOMPARE(applyFlags(flags, offsets, 0x1000, memory).count(FlagStatus::VerifyFailed), 1);
    }

    void classifiesEveryFailureSeparately()
    {
        FakeMemory memory;
        memory.failingWrites.insert(0x1000 + 0x30);
        const OffsetMap offsets{
            {QStringLiteral("Ok"), 0x10},
            {QStringLiteral("Text"), 0x20},
            {QStringLiteral("Denied"), 0x30},
        };
        QJsonObject flags;
        flags.insert(QStringLiteral("Ok"), QStringLiteral("5"));
        flags.insert(QStringLiteral("NoOffset"), QStringLiteral("5"));
        flags.insert(QStringLiteral("Text"), QStringLiteral("not a number"));
        flags.insert(QStringLiteral("Denied"), QStringLiteral("1"));

        const ApplyReport report = applyFlags(flags, offsets, 0x1000, memory);
        QCOMPARE(report.applied(), 1);
        QCOMPARE(report.count(FlagStatus::MissingOffset), 1);
        QCOMPARE(report.count(FlagStatus::InvalidValue), 1);
        QCOMPARE(report.count(FlagStatus::WriteFailed), 1);
        QCOMPARE(report.count(FlagStatus::VerifyFailed), 0);
        QCOMPARE(report.failed(), 3);
    }

    void perFlagResultsExplainEachOutcome()
    {
        FakeMemory memory;
        memory.failingWrites.insert(0x1000 + 0x30);
        memory.ignoredWrites.insert(0x1000 + 0x40);
        memory.memory.insert(0x1000 + 0x40, 9);
        const OffsetMap offsets{
            {QStringLiteral("Ok"), 0x10},
            {QStringLiteral("Text"), 0x20},
            {QStringLiteral("Denied"), 0x30},
            {QStringLiteral("Stale"), 0x40},
        };
        QJsonObject flags;
        flags.insert(QStringLiteral("Ok"), QStringLiteral("5"));
        flags.insert(QStringLiteral("NoOffset"), QStringLiteral("5"));
        flags.insert(QStringLiteral("Text"), QStringLiteral("abc"));
        flags.insert(QStringLiteral("Denied"), QStringLiteral("1"));
        flags.insert(QStringLiteral("Stale"), QStringLiteral("1"));

        const ApplyReport report = applyFlags(flags, offsets, 0x1000, memory);

        QCOMPARE(report.results.size(), 5);
        QCOMPARE(report.results.value(QStringLiteral("Ok")).status, FlagStatus::Applied);
        QCOMPARE(report.results.value(QStringLiteral("NoOffset")).status, FlagStatus::MissingOffset);
        QCOMPARE(report.results.value(QStringLiteral("Text")).status, FlagStatus::InvalidValue);
        QCOMPARE(report.results.value(QStringLiteral("Denied")).status, FlagStatus::WriteFailed);
        QCOMPARE(report.results.value(QStringLiteral("Stale")).status, FlagStatus::VerifyFailed);

        for (const FlagResult &result : report.results)
            QVERIFY(!result.detail.isEmpty());
        QVERIFY(report.results.value(QStringLiteral("Stale")).detail.contains(QStringLiteral("9"))); // the value read back
        QVERIFY(report.results.value(QStringLiteral("NoOffset")).detail.contains(QStringLiteral("NoOffset")));
        QVERIFY(report.results.value(QStringLiteral("Text")).detail.contains(QStringLiteral("abc")));
    }

    void emptyOffsetsTableFailsEveryFlagAsMissingOffset()
    {
        FakeMemory memory;
        QJsonObject flags;
        flags.insert(QStringLiteral("Alpha"), QStringLiteral("1"));

        const ApplyReport report = applyFlags(flags, {}, 0x1000, memory);
        QCOMPARE(report.count(FlagStatus::MissingOffset), 1);
        QVERIFY(report.results.value(QStringLiteral("Alpha")).detail.contains(QStringLiteral("empty")));
    }

    void flagsWithLowercaseNamesAndVerbatimPrefixedNamesAreFound()
    {
        FakeMemory memory;
        const OffsetMap offsets = parseOffsetsHeader(QStringLiteral(
            "inline constexpr std::uintptr_t kCageHoleNum = 0x10;\n"
            "inline constexpr std::uintptr_t FFlagMigrateCameraFovDatamodel = 0x20;\n"));
        QJsonObject flags;
        flags.insert(QStringLiteral("FIntkCageHoleNum"), QStringLiteral("5"));
        flags.insert(QStringLiteral("kCageHoleNum"), QStringLiteral("6"));
        flags.insert(QStringLiteral("FFlagMigrateCameraFovDatamodel"), QStringLiteral("true"));
        flags.insert(QStringLiteral("MigrateCameraFovDatamodel"), QStringLiteral("true"));

        const ApplyReport report = applyFlags(flags, offsets, 0x1000, memory);
        QCOMPARE(report.applied(), 4);
        QCOMPARE(report.count(FlagStatus::MissingOffset), 0);
    }

    void resultsAreKeyedByOriginalFlagName()
    {
        FakeMemory memory;
        const OffsetMap offsets{{QStringLiteral("Alpha"), 0x8}};
        QJsonObject flags;
        flags.insert(QStringLiteral("FFlagAlpha"), QStringLiteral("1"));

        const ApplyReport report = applyFlags(flags, offsets, 0x100, memory);
        QVERIFY(report.results.contains(QStringLiteral("FFlagAlpha")));
    }

    void summaryIsShortAndClear()
    {
        ApplyReport all;
        all.results.insert(QStringLiteral("A"), {FlagStatus::Applied, {}});
        all.results.insert(QStringLiteral("B"), {FlagStatus::Applied, {}});
        all.results.insert(QStringLiteral("C"), {FlagStatus::Applied, {}});
        QCOMPARE(all.summary(), QStringLiteral("Applied 3/3 flags"));

        ApplyReport mixed;
        mixed.results.insert(QStringLiteral("A"), {FlagStatus::Applied, {}});
        mixed.results.insert(QStringLiteral("B"), {FlagStatus::Applied, {}});
        mixed.results.insert(QStringLiteral("C"), {FlagStatus::MissingOffset, {}});
        mixed.results.insert(QStringLiteral("D"), {FlagStatus::InvalidValue, {}});
        QCOMPARE(mixed.summary(), QStringLiteral("Applied 2/4 flags · 2 failed"));
    }

    void isFailedClassification()
    {
        QVERIFY(isFailed(FlagStatus::MissingOffset));
        QVERIFY(isFailed(FlagStatus::InvalidValue));
        QVERIFY(isFailed(FlagStatus::VerifyFailed));
        QVERIFY(isFailed(FlagStatus::WriteFailed));
        QVERIFY(!isFailed(FlagStatus::Pending));
        QVERIFY(!isFailed(FlagStatus::Applied));
    }

    void prefixedFlagNamesStillMatchOffsets()
    {
        FakeMemory memory;
        const OffsetMap offsets{{QStringLiteral("Alpha"), 0x8}};
        QJsonObject flags;
        flags.insert(QStringLiteral("FFlagAlpha"), QStringLiteral("1"));

        QCOMPARE(applyFlags(flags, offsets, 0x100, memory).applied(), 1);
    }

    void rejectsAddressOverflow()
    {
        FakeMemory memory;
        const quintptr max = std::numeric_limits<quintptr>::max();
        const OffsetMap offsets{{QStringLiteral("Big"), max}};
        QJsonObject flags;
        flags.insert(QStringLiteral("Big"), QStringLiteral("1"));

        const ApplyReport report = applyFlags(flags, offsets, 0x10, memory);
        QCOMPARE(report.count(FlagStatus::WriteFailed), 1);
        QVERIFY(memory.memory.isEmpty());
    }

    void emptyInputProducesEmptyReport()
    {
        FakeMemory memory;
        const ApplyReport report = applyFlags({}, {}, 0x1000, memory);
        QCOMPARE(report.total(), 0);
        QCOMPARE(report.summary(), QStringLiteral("Applied 0/0 flags"));
    }
};

QTEST_GUILESS_MAIN(FlagApplierTests)
#include "test_flag_applier.moc"
