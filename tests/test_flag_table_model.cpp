#include "core/flags/flag_filter_proxy_model.h"
#include "core/flags/flag_store.h"
#include "core/flags/flag_table_model.h"

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
        QCOMPARE(model.columnCount(), 3);
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

        QVERIFY(!model.setData(model.index(0, FlagTableModel::ValueColumn), QStringLiteral("   "))); // empty
        QVERIFY(!model.setData(model.index(0, FlagTableModel::NameColumn), QStringLiteral("x")));    // wrong column
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

    // ---- Status column: only three displayed states ---------------------

    void statusIsPendingBeforeAnyApply()
    {
        FlagStore store;
        store.merge(sampleFlags());
        FlagTableModel model(&store);

        for (int row = 0; row < model.rowCount(); ++row) {
            QCOMPARE(model.index(row, FlagTableModel::StatusColumn).data().toString(),
                     QStringLiteral("— Pending"));
        }
        QCOMPARE(model.failedCount(), 0);
    }

    void applyResultsMapToAppliedOrNotApplied()
    {
        FlagStore store;
        store.merge(sampleFlags()); // Alpha, Mid, Zeta
        FlagTableModel model(&store);

        QHash<QString, FlagResult> results;
        results.insert(QStringLiteral("Alpha"), {FlagStatus::Applied, QStringLiteral("ok")});
        results.insert(QStringLiteral("Mid"), {FlagStatus::InvalidValue, QStringLiteral("x")});
        results.insert(QStringLiteral("Zeta"), {FlagStatus::MissingOffset, QStringLiteral("x")});
        model.setApplyResults(results);

        auto text = [&](int row) { return model.index(row, FlagTableModel::StatusColumn).data().toString(); };
        QCOMPARE(text(0), QStringLiteral("✓ Applied"));
        QCOMPARE(text(1), QStringLiteral("✗ Failed"));
        QCOMPARE(text(2), QStringLiteral("✗ Failed")); // every failure reason shares one label
        QCOMPARE(model.failedCount(), 2);

        // The different reasons can still be told apart through StatusRole (not shown in the table).
        QCOMPARE(model.statusAt(1), FlagStatus::InvalidValue);
        QCOMPARE(model.statusAt(2), FlagStatus::MissingOffset);
        // There are no explanatory tooltips or name suggestions on the status and name columns.
        QVERIFY(!model.index(2, FlagTableModel::StatusColumn).data(Qt::ToolTipRole).isValid());
        QVERIFY(!model.index(2, FlagTableModel::NameColumn).data(Qt::ToolTipRole).isValid());
    }

    void setApplyResultsEmitsStatusColumnChange()
    {
        FlagStore store;
        store.merge(sampleFlags());
        FlagTableModel model(&store);
        QSignalSpy changed(&model, &QAbstractItemModel::dataChanged);

        QHash<QString, FlagResult> results;
        results.insert(QStringLiteral("Alpha"), {FlagStatus::Applied, QString()});
        model.setApplyResults(results);

        QCOMPARE(changed.count(), 1);
        QCOMPARE(changed.at(0).at(0).value<QModelIndex>().column(), int(FlagTableModel::StatusColumn));
    }

    void clearApplyResultsReturnsToPending()
    {
        FlagStore store;
        store.merge(sampleFlags());
        FlagTableModel model(&store);

        QHash<QString, FlagResult> results;
        results.insert(QStringLiteral("Alpha"), {FlagStatus::VerifyFailed, QString()});
        model.setApplyResults(results);
        QCOMPARE(model.failedCount(), 1);

        model.clearApplyResults();
        QCOMPARE(model.failedCount(), 0);
        QCOMPARE(model.index(0, FlagTableModel::StatusColumn).data().toString(), QStringLiteral("— Pending"));
    }

    void editingValueDiscardsThatFlagsApplyResult()
    {
        FlagStore store;
        store.merge(sampleFlags());
        FlagTableModel model(&store);

        QHash<QString, FlagResult> results;
        results.insert(QStringLiteral("Alpha"), {FlagStatus::Applied, QString()});
        results.insert(QStringLiteral("Zeta"), {FlagStatus::Applied, QString()});
        model.setApplyResults(results);

        QVERIFY(model.setData(model.index(0, FlagTableModel::ValueColumn), QStringLiteral("false"))); // Alpha

        QCOMPARE(model.index(0, FlagTableModel::StatusColumn).data().toString(), QStringLiteral("— Pending"));
        QCOMPARE(model.index(2, FlagTableModel::StatusColumn).data().toString(), QStringLiteral("✓ Applied"));
    }

    void reloadKeepsResultsOnlyForSurvivingFlags()
    {
        FlagStore store;
        store.merge(sampleFlags());
        FlagTableModel model(&store);

        QHash<QString, FlagResult> results;
        results.insert(QStringLiteral("Alpha"), {FlagStatus::Applied, QString()});
        results.insert(QStringLiteral("Zeta"), {FlagStatus::Applied, QString()});
        model.setApplyResults(results);

        store.remove({QStringLiteral("Zeta")});
        model.reload();
        QJsonObject again;
        again.insert(QStringLiteral("Zeta"), 1); // re-added: must not "inherit" the old result
        store.merge(again);
        model.reload();

        QCOMPARE(model.index(0, FlagTableModel::StatusColumn).data().toString(), QStringLiteral("✓ Applied"));
        QCOMPARE(model.index(2, FlagTableModel::StatusColumn).data().toString(), QStringLiteral("— Pending"));
    }

    void proxyFailedOnlyAndStatusSort()
    {
        FlagStore store;
        store.merge(sampleFlags()); // Alpha, Mid, Zeta
        FlagTableModel model(&store);
        FlagFilterProxyModel proxy;
        proxy.setSourceModel(&model);

        QHash<QString, FlagResult> results;
        results.insert(QStringLiteral("Alpha"), {FlagStatus::Applied, QString()});
        results.insert(QStringLiteral("Zeta"), {FlagStatus::WriteFailed, QString()});
        model.setApplyResults(results); // Mid: pending

        QCOMPARE(proxy.rowCount(), 3);
        proxy.setFailedOnly(true);
        QCOMPARE(proxy.rowCount(), 1); // only Zeta failed
        QCOMPARE(proxy.index(0, FlagTableModel::NameColumn).data().toString(), QStringLiteral("Zeta"));

        proxy.setFailedOnly(false);
        proxy.sort(FlagTableModel::StatusColumn, Qt::AscendingOrder);
        // Failed (Zeta) -> pending (Mid) -> applied (Alpha).
        QCOMPARE(proxy.index(0, FlagTableModel::NameColumn).data().toString(), QStringLiteral("Zeta"));
        QCOMPARE(proxy.index(1, FlagTableModel::NameColumn).data().toString(), QStringLiteral("Mid"));
        QCOMPARE(proxy.index(2, FlagTableModel::NameColumn).data().toString(), QStringLiteral("Alpha"));

        // Typing the status text into the search box filters as well.
        proxy.setFilterFixedString(QStringLiteral("Applied"));
        QCOMPARE(proxy.rowCount(), 1);
        QCOMPARE(proxy.index(0, FlagTableModel::NameColumn).data().toString(), QStringLiteral("Alpha"));
    }

    void proxyReactsWhenApplyResultsChange()
    {
        FlagStore store;
        store.merge(sampleFlags());
        FlagTableModel model(&store);
        FlagFilterProxyModel proxy;
        proxy.setSourceModel(&model);
        proxy.setFailedOnly(true);
        QCOMPARE(proxy.rowCount(), 0); // nothing applied yet: nothing has failed

        QHash<QString, FlagResult> results;
        results.insert(QStringLiteral("Zeta"), {FlagStatus::WriteFailed, QString()});
        model.setApplyResults(results);

        QCOMPARE(proxy.rowCount(), 1);
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

        proxy.setFilterFixedString(QStringLiteral("some TEXT")); // matches by value
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
