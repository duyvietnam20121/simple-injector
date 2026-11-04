#include "core/flag_store.h"
#include "core/flag_table_model.h"

#include <QAbstractItemModelTester>
#include <QSignalSpy>
#include <QSortFilterProxyModel>
#include <QtTest>

using namespace SimpleInjector;

namespace {

QJsonObject sampleFlags()
{
    QJsonObject flags;
    flags.insert(QStringLiteral("Zeta"), QStringLiteral("1"));
    flags.insert(QStringLiteral("Alpha"), QStringLiteral("true"));
    flags.insert(QStringLiteral("Mid"), QStringLiteral("some text"));
    return flags;
}

} // namespace

class FlagTableModelTests final : public QObject {
    Q_OBJECT

private slots:
    void satisfiesModelContract()
    {
        FlagStore store;
        store.merge(sampleFlags());
        FlagTableModel model(&store);
        QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);

        model.reload();
        store.remove({QStringLiteral("Mid")});
        model.reload();
    }

    void exposesSortedNamesAndValues()
    {
        FlagStore store;
        store.merge(sampleFlags());
        FlagTableModel model(&store);

        QCOMPARE(model.rowCount(), 3);
        QCOMPARE(model.columnCount(), 2);
        QCOMPARE(model.nameAt(0), QStringLiteral("Alpha"));
        QCOMPARE(model.index(0, FlagTableModel::ValueColumn).data().toString(), QStringLiteral("true"));
        QCOMPARE(model.nameAt(99), QString());
    }

    void onlyValueColumnIsEditable()
    {
        FlagStore store;
        store.merge(sampleFlags());
        FlagTableModel model(&store);

        QVERIFY(!(model.flags(model.index(0, FlagTableModel::NameColumn)) & Qt::ItemIsEditable));
        QVERIFY(model.flags(model.index(0, FlagTableModel::ValueColumn)) & Qt::ItemIsEditable);
    }

    void setDataUpdatesStoreAndEmitsSignals()
    {
        FlagStore store;
        store.merge(sampleFlags());
        FlagTableModel model(&store);
        QSignalSpy edited(&model, &FlagTableModel::valueEdited);
        QSignalSpy changed(&model, &QAbstractItemModel::dataChanged);

        const QModelIndex index = model.index(0, FlagTableModel::ValueColumn); // Alpha
        QVERIFY(model.setData(index, QStringLiteral("  false  ")));

        QCOMPARE(store.entries().value(QStringLiteral("Alpha")).toString(), QStringLiteral("false"));
        QCOMPARE(edited.count(), 1);
        QCOMPARE(edited.at(0).at(0).toString(), QStringLiteral("Alpha"));
        QCOMPARE(edited.at(0).at(1).toString(), QStringLiteral("false"));
        QCOMPARE(changed.count(), 1);
    }

    void setDataRejectsInvalidEdits()
    {
        FlagStore store;
        store.merge(sampleFlags());
        FlagTableModel model(&store);
        QSignalSpy edited(&model, &FlagTableModel::valueEdited);

        QVERIFY(!model.setData(model.index(0, FlagTableModel::ValueColumn), QStringLiteral("   "))); // rỗng
        QVERIFY(!model.setData(model.index(0, FlagTableModel::NameColumn), QStringLiteral("x")));    // sai cột
        QVERIFY(!model.setData(model.index(0, FlagTableModel::ValueColumn), QStringLiteral("x"), Qt::DisplayRole));
        QCOMPARE(edited.count(), 0);
        QCOMPARE(store.entries().value(QStringLiteral("Alpha")).toString(), QStringLiteral("true"));
    }

    void unchangedValueSucceedsWithoutSignal()
    {
        FlagStore store;
        store.merge(sampleFlags());
        FlagTableModel model(&store);
        QSignalSpy edited(&model, &FlagTableModel::valueEdited);

        QVERIFY(model.setData(model.index(0, FlagTableModel::ValueColumn), QStringLiteral("true")));
        QCOMPARE(edited.count(), 0);
    }

    void tooltipSaysWhetherValueCanBeApplied()
    {
        FlagStore store;
        store.merge(sampleFlags());
        FlagTableModel model(&store);

        const QString applicable = model.index(0, FlagTableModel::ValueColumn).data(Qt::ToolTipRole).toString();
        const QString storedOnly = model.index(1, FlagTableModel::ValueColumn).data(Qt::ToolTipRole).toString(); // Mid
        QVERIFY(applicable.contains(QStringLiteral("Có thể Apply")));
        QVERIFY(storedOnly.contains(QStringLiteral("Chỉ lưu")));
    }

    void reloadReflectsExternalStoreChanges()
    {
        FlagStore store;
        store.merge(sampleFlags());
        FlagTableModel model(&store);
        QSignalSpy resets(&model, &QAbstractItemModel::modelReset);

        QJsonObject extra;
        extra.insert(QStringLiteral("Beta"), 5);
        store.merge(extra);
        model.reload();

        QCOMPARE(resets.count(), 1);
        QCOMPARE(model.rowCount(), 4);
        QCOMPARE(model.nameAt(1), QStringLiteral("Beta"));
    }

    void proxyFiltersNamesAndValuesCaseInsensitively()
    {
        FlagStore store;
        store.merge(sampleFlags());
        FlagTableModel model(&store);
        QSortFilterProxyModel proxy;
        proxy.setSourceModel(&model);
        proxy.setFilterKeyColumn(-1);
        proxy.setFilterCaseSensitivity(Qt::CaseInsensitive);

        proxy.setFilterFixedString(QStringLiteral("ALPHA"));
        QCOMPARE(proxy.rowCount(), 1);

        proxy.setFilterFixedString(QStringLiteral("some TEXT")); // khớp theo giá trị
        QCOMPARE(proxy.rowCount(), 1);
        QCOMPARE(proxy.index(0, 0).data().toString(), QStringLiteral("Mid"));

        proxy.setFilterFixedString(QString());
        QCOMPARE(proxy.rowCount(), 3);
    }

    void proxySortsByValueDescending()
    {
        FlagStore store;
        store.merge(sampleFlags());
        FlagTableModel model(&store);
        QSortFilterProxyModel proxy;
        proxy.setSourceModel(&model);
        proxy.sort(FlagTableModel::NameColumn, Qt::DescendingOrder);

        QCOMPARE(proxy.index(0, 0).data().toString(), QStringLiteral("Zeta"));
        QCOMPARE(proxy.index(2, 0).data().toString(), QStringLiteral("Alpha"));
    }

    void handlesThousandsOfRows()
    {
        FlagStore store;
        QJsonObject many;
        for (int i = 0; i < 5000; ++i)
            many.insert(QStringLiteral("Flag%1").arg(i, 5, 10, QLatin1Char('0')), i);
        store.merge(many);

        FlagTableModel model(&store);
        QSortFilterProxyModel proxy;
        proxy.setSourceModel(&model);
        proxy.setFilterKeyColumn(-1);

        proxy.setFilterFixedString(QStringLiteral("Flag04999"));
        QCOMPARE(model.rowCount(), 5000);
        QCOMPARE(proxy.rowCount(), 1);
    }
};

QTEST_GUILESS_MAIN(FlagTableModelTests)
#include "test_flag_table_model.moc"
