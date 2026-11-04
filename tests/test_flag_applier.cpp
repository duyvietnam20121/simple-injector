#include "core/flag_applier.h"

#include <QMap>
#include <QSet>
#include <QtTest>

#include <limits>

using namespace SimpleInjector;

namespace {

/// Bộ nhớ giả. Có thể giả lập: ghi thất bại, game ghi đè giá trị (ghi OK nhưng đọc ra khác),
/// hoặc vùng nhớ không đọc được.
class FakeWriter final : public IMemoryWriter {
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
    QSet<quintptr> ignoredWrites; ///< Ghi báo OK nhưng giá trị không đổi (offset sai / bị ghi đè).
    QSet<quintptr> unreadable;
};

} // namespace

class FlagApplierTests final : public QObject {
    Q_OBJECT

private slots:
    void writesAtImageBasePlusOffsetAndVerifies()
    {
        FakeWriter writer;
        const OffsetMap offsets{{QStringLiteral("Alpha"), 0x10}, {QStringLiteral("Beta"), 0x20}};
        QJsonObject flags;
        flags.insert(QStringLiteral("Alpha"), QStringLiteral("true"));
        flags.insert(QStringLiteral("Beta"), QStringLiteral("-7"));

        const ApplyReport report = applyFlags(flags, offsets, 0x1000, writer);
        QCOMPARE(report.applied, 2);
        QCOMPARE(report.verifyFailed, 0);
        QCOMPARE(report.total(), 2);
        QCOMPARE(writer.memory.value(0x1010), qint32(1));
        QCOMPARE(writer.memory.value(0x1020), qint32(-7));
    }

    void readBackMismatchIsReportedSeparately()
    {
        FakeWriter writer;
        writer.ignoredWrites.insert(0x1000 + 0x10);
        writer.memory.insert(0x1000 + 0x10, 99); // giá trị cũ còn nguyên
        const OffsetMap offsets{{QStringLiteral("Stale"), 0x10}};
        QJsonObject flags;
        flags.insert(QStringLiteral("Stale"), QStringLiteral("1"));

        const ApplyReport report = applyFlags(flags, offsets, 0x1000, writer);
        QCOMPARE(report.applied, 0);
        QCOMPARE(report.verifyFailed, 1);
        QVERIFY(report.summary().contains(QStringLiteral("không khớp")));
    }

    void unreadableMemoryCountsAsVerifyFailure()
    {
        FakeWriter writer;
        writer.unreadable.insert(0x1000 + 0x10);
        const OffsetMap offsets{{QStringLiteral("Hidden"), 0x10}};
        QJsonObject flags;
        flags.insert(QStringLiteral("Hidden"), QStringLiteral("1"));

        QCOMPARE(applyFlags(flags, offsets, 0x1000, writer).verifyFailed, 1);
    }

    void classifiesEveryFailureSeparately()
    {
        FakeWriter writer;
        writer.failingWrites.insert(0x1000 + 0x30);
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

        const ApplyReport report = applyFlags(flags, offsets, 0x1000, writer);
        QCOMPARE(report.applied, 1);
        QCOMPARE(report.missingOffset, 1);
        QCOMPARE(report.invalidValue, 1);
        QCOMPARE(report.writeFailed, 1);
        QCOMPARE(report.verifyFailed, 0);
    }

    void prefixedFlagNamesStillMatchOffsets()
    {
        FakeWriter writer;
        const OffsetMap offsets{{QStringLiteral("Alpha"), 0x8}};
        QJsonObject flags;
        flags.insert(QStringLiteral("FFlagAlpha"), QStringLiteral("1"));

        QCOMPARE(applyFlags(flags, offsets, 0x100, writer).applied, 1);
    }

    void rejectsAddressOverflow()
    {
        FakeWriter writer;
        const quintptr max = std::numeric_limits<quintptr>::max();
        const OffsetMap offsets{{QStringLiteral("Big"), max}};
        QJsonObject flags;
        flags.insert(QStringLiteral("Big"), QStringLiteral("1"));

        const ApplyReport report = applyFlags(flags, offsets, 0x10, writer);
        QCOMPARE(report.writeFailed, 1);
        QVERIFY(writer.memory.isEmpty());
    }

    void emptyInputProducesEmptyReport()
    {
        FakeWriter writer;
        QCOMPARE(applyFlags({}, {}, 0x1000, writer).total(), 0);
    }
};

QTEST_GUILESS_MAIN(FlagApplierTests)
#include "test_flag_applier.moc"
