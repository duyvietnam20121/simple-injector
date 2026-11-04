#include "core/flag_applier.h"

#include <QMap>
#include <QtTest>

using namespace SimpleInjector;

namespace {

class FakeWriter final : public IMemoryWriter {
public:
    bool writeInt32(quintptr address, qint32 value) override
    {
        if (failingAddresses.contains(address))
            return false;
        written.insert(address, value);
        return true;
    }

    QMap<quintptr, qint32> written;
    QSet<quintptr> failingAddresses;
};

} // namespace

class FlagApplierTests final : public QObject {
    Q_OBJECT

private slots:
    void writesAtImageBasePlusOffset()
    {
        FakeWriter writer;
        const OffsetMap offsets{{QStringLiteral("Alpha"), 0x10}, {QStringLiteral("Beta"), 0x20}};
        QJsonObject flags;
        flags.insert(QStringLiteral("Alpha"), QStringLiteral("true"));
        flags.insert(QStringLiteral("Beta"), QStringLiteral("-7"));

        const ApplyReport report = applyFlags(flags, offsets, 0x1000, writer);
        QCOMPARE(report.applied, 2);
        QCOMPARE(report.total(), 2);
        QCOMPARE(writer.written.value(0x1010), qint32(1));
        QCOMPARE(writer.written.value(0x1020), qint32(-7));
    }

    void classifiesEveryFailureSeparately()
    {
        FakeWriter writer;
        writer.failingAddresses.insert(0x1000 + 0x30);
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
        QVERIFY(writer.written.isEmpty());
    }

    void emptyInputProducesEmptyReport()
    {
        FakeWriter writer;
        QCOMPARE(applyFlags({}, {}, 0x1000, writer).total(), 0);
    }
};

QTEST_GUILESS_MAIN(FlagApplierTests)
#include "test_flag_applier.moc"
