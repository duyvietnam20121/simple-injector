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

    // ---- Cột trạng thái: chỉ 3 trạng thái hiển thị ------------------------

    void statusIsPendingBeforeAnyApply()
    {
        FlagStore store;
        store.merge(sampleFlags());
        FlagTableModel model(&store);

        for (int row = 0; row < model.rowCount(); ++row) {
            QCOMPARE(model.index(row, FlagTableModel::StatusColumn).data().toString(),
                     QStringLiteral("— Chưa áp dụng"));
        }
        QCOMPARE(model.problemCount(), 0);
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
        QCOMPARE(text(0), QStringLiteral("✓ Đã áp dụng"));
        QCOMPARE(text(1), QStringLiteral("✗ Không áp dụng"));
        QCOMPARE(text(2), QStringLiteral("✗ Không áp dụng")); // mọi nguyên nhân thất bại đều gộp một nhãn
        QCOMPARE(model.problemCount(), 2);

        // Các lý do khác nhau vẫn phân biệt được qua StatusRole (không hiển thị trong bảng).
        QCOMPARE(model.statusAt(1), FlagStatus::InvalidValue);
        QCOMPARE(model.statusAt(2), FlagStatus::MissingOffset);
        // Không còn tooltip giải thích hay gợi ý tên ở cột trạng thái và cột tên.
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
        QCOMPARE(model.problemCount(), 1);

        model.clearApplyResults();
        QCOMPARE(model.problemCount(), 0);
        QCOMPARE(model.index(0, FlagTableModel::StatusColumn).data().toString(), QStringLiteral("— Chưa áp dụng"));
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

        QCOMPARE(model.index(0, FlagTableModel::StatusColumn).data().toString(), QStringLiteral("— Chưa áp dụng"));
        QCOMPARE(model.index(2, FlagTableModel::StatusColumn).data().toString(), QStringLiteral("✓ Đã áp dụng"));
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
        again.insert(QStringLiteral("Zeta"), 1); // thêm lại: không được "thừa hưởng" kết quả cũ
        store.merge(again);
        model.reload();

        QCOMPARE(model.index(0, FlagTableModel::StatusColumn).data().toString(), QStringLiteral("✓ Đã áp dụng"));
        QCOMPARE(model.index(2, FlagTableModel::StatusColumn).data().toString(), QStringLiteral("— Chưa áp dụng"));
    }

    void proxyNotAppliedOnlyAndStatusSort()
    {
        FlagStore store;
        store.merge(sampleFlags()); // Alpha, Mid, Zeta
        FlagTableModel model(&store);
        FlagFilterProxyModel proxy;
        proxy.setSourceModel(&model);

        QHash<QString, FlagResult> results;
        results.insert(QStringLiteral("Alpha"), {FlagStatus::Applied, QString()});
        results.insert(QStringLiteral("Zeta"), {FlagStatus::WriteFailed, QString()});
        model.setApplyResults(results); // Mid: chưa áp dụng

        QCOMPARE(proxy.rowCount(), 3);
        proxy.setProblemsOnly(true);
        QCOMPARE(proxy.rowCount(), 1); // chỉ Zeta "không áp dụng"
        QCOMPARE(proxy.index(0, FlagTableModel::NameColumn).data().toString(), QStringLiteral("Zeta"));

        proxy.setProblemsOnly(false);
        proxy.sort(FlagTableModel::StatusColumn, Qt::AscendingOrder);
        // Không áp dụng (Zeta) -> chưa áp dụng (Mid) -> đã áp dụng (Alpha).
        QCOMPARE(proxy.index(0, FlagTableModel::NameColumn).data().toString(), QStringLiteral("Zeta"));
        QCOMPARE(proxy.index(1, FlagTableModel::NameColumn).data().toString(), QStringLiteral("Mid"));
        QCOMPARE(proxy.index(2, FlagTableModel::NameColumn).data().toString(), QStringLiteral("Alpha"));

        // Gõ chữ trạng thái vào ô tìm kiếm cũng lọc được.
        proxy.setFilterFixedString(QStringLiteral("Đã áp dụng"));
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
        proxy.setProblemsOnly(true);
        QCOMPARE(proxy.rowCount(), 0); // chưa Apply: chưa có gì "không áp dụng"

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
