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

    // ---- Cột trạng thái ------------------------------------------------------

    void statusIsUnknownWithoutOffsets()
    {
        FlagStore store;
        store.merge(sampleFlags());
        FlagTableModel model(&store);

        for (int row = 0; row < model.rowCount(); ++row)
            QCOMPARE(model.statusAt(row), FlagStatus::Unknown);
        QCOMPARE(model.problemCount(), 0);
    }

    void statusIsPredictedFromOffsets()
    {
        FlagStore store;
        store.merge(sampleFlags()); // Alpha=true, Mid="some text", Zeta=1
        OffsetMap offsets{{QStringLiteral("Alpha"), 0x10}, {QStringLiteral("Mid"), 0x20}};
        FlagTableModel model(&store);
        model.setOffsets(&offsets);

        QCOMPARE(model.statusAt(0), FlagStatus::Ready);         // Alpha: có offset, giá trị hợp lệ
        QCOMPARE(model.statusAt(1), FlagStatus::InvalidValue);  // Mid: có offset nhưng là chuỗi
        QCOMPARE(model.statusAt(2), FlagStatus::MissingOffset); // Zeta: không có offset
        QCOMPARE(model.problemCount(), 2);

        const QModelIndex zetaStatus = model.index(2, FlagTableModel::StatusColumn);
        QVERIFY(zetaStatus.data(Qt::DisplayRole).toString().contains(QStringLiteral("Không có offset")));
        QVERIFY(zetaStatus.data(Qt::ToolTipRole).toString().contains(QStringLiteral("Zeta")));
    }

    void missingOffsetTooltipSuggestsCloseNames()
    {
        FlagStore store;
        QJsonObject flags;
        flags.insert(QStringLiteral("DebugDisplayFPS"), true); // sai hoa/thường so với bảng offsets
        flags.insert(QStringLiteral("TotallyUnknown"), true);
        store.merge(flags);
        OffsetMap offsets{{QStringLiteral("DebugDisplayFps"), 0x10}};
        FlagTableModel model(&store);
        model.setOffsets(&offsets);

        QCOMPARE(model.statusAt(0), FlagStatus::MissingOffset);
        const QString hinted = model.index(0, FlagTableModel::StatusColumn).data(Qt::ToolTipRole).toString();
        QVERIFY(hinted.contains(QStringLiteral("Gợi ý")));
        QVERIFY(hinted.contains(QStringLiteral("DebugDisplayFps")));
        QVERIFY(model.index(0, FlagTableModel::NameColumn).data(Qt::ToolTipRole).toString()
                    .contains(QStringLiteral("DebugDisplayFps")));

        const QString plain = model.index(1, FlagTableModel::StatusColumn).data(Qt::ToolTipRole).toString();
        QVERIFY(!plain.contains(QStringLiteral("Gợi ý"))); // không có gì gần giống thì không gợi ý
    }

    void refreshStatusesFollowsOffsetsContent()
    {
        FlagStore store;
        store.merge(sampleFlags());
        OffsetMap offsets;
        FlagTableModel model(&store);
        model.setOffsets(&offsets);
        QCOMPARE(model.statusAt(0), FlagStatus::Unknown);

        QSignalSpy changed(&model, &QAbstractItemModel::dataChanged);
        offsets.insert(QStringLiteral("Alpha"), 0x10); // bảng offsets đổi bên ngoài
        model.refreshStatuses();

        QCOMPARE(changed.count(), 1);
        QCOMPARE(changed.at(0).at(0).value<QModelIndex>().column(), int(FlagTableModel::StatusColumn));
        QCOMPARE(model.statusAt(0), FlagStatus::Ready);
        QCOMPARE(model.statusAt(1), FlagStatus::MissingOffset);
    }

    void applyResultsOverridePredictionAndClear()
    {
        FlagStore store;
        store.merge(sampleFlags());
        OffsetMap offsets{{QStringLiteral("Alpha"), 0x10}};
        FlagTableModel model(&store);
        model.setOffsets(&offsets);

        QHash<QString, FlagResult> results;
        results.insert(QStringLiteral("Alpha"), {FlagStatus::VerifyFailed, QStringLiteral("đọc lại ra 0")});
        model.setApplyResults(results);

        QCOMPARE(model.statusAt(0), FlagStatus::VerifyFailed);
        QCOMPARE(model.index(0, FlagTableModel::StatusColumn).data(Qt::ToolTipRole).toString(),
                 QStringLiteral("đọc lại ra 0"));

        model.clearApplyResults();
        QCOMPARE(model.statusAt(0), FlagStatus::Ready); // quay về dự đoán
    }

    void editingValueDiscardsThatFlagsApplyResult()
    {
        FlagStore store;
        store.merge(sampleFlags());
        OffsetMap offsets{{QStringLiteral("Alpha"), 0x10}, {QStringLiteral("Zeta"), 0x20}};
        FlagTableModel model(&store);
        model.setOffsets(&offsets);

        QHash<QString, FlagResult> results;
        results.insert(QStringLiteral("Alpha"), {FlagStatus::Applied, QStringLiteral("ok")});
        results.insert(QStringLiteral("Zeta"), {FlagStatus::Applied, QStringLiteral("ok")});
        model.setApplyResults(results);

        QVERIFY(model.setData(model.index(0, FlagTableModel::ValueColumn), QStringLiteral("false"))); // Alpha

        QCOMPARE(model.statusAt(0), FlagStatus::Ready);   // Alpha đổi giá trị -> chưa Apply lại
        QCOMPARE(model.statusAt(2), FlagStatus::Applied); // Zeta không bị ảnh hưởng
    }

    void reloadKeepsResultsOnlyForSurvivingFlags()
    {
        FlagStore store;
        store.merge(sampleFlags());
        OffsetMap offsets{{QStringLiteral("Alpha"), 0x10}, {QStringLiteral("Zeta"), 0x20}};
        FlagTableModel model(&store);
        model.setOffsets(&offsets);

        QHash<QString, FlagResult> results;
        results.insert(QStringLiteral("Alpha"), {FlagStatus::Applied, QStringLiteral("ok")});
        results.insert(QStringLiteral("Zeta"), {FlagStatus::Applied, QStringLiteral("ok")});
        model.setApplyResults(results);

        store.remove({QStringLiteral("Zeta")});
        model.reload();
        QJsonObject again;
        again.insert(QStringLiteral("Zeta"), 1); // thêm lại: không được "thừa hưởng" kết quả cũ
        store.merge(again);
        model.reload();

        QCOMPARE(model.statusAt(0), FlagStatus::Applied); // Alpha vẫn giữ
        QCOMPARE(model.statusAt(2), FlagStatus::Ready);   // Zeta bắt đầu lại từ dự đoán
    }

    void proxyProblemsOnlyAndStatusSort()
    {
        FlagStore store;
        store.merge(sampleFlags()); // Alpha=true, Mid=text, Zeta=1
        OffsetMap offsets{{QStringLiteral("Alpha"), 0x10}, {QStringLiteral("Mid"), 0x20}};
        FlagTableModel model(&store);
        model.setOffsets(&offsets);
        FlagFilterProxyModel proxy;
        proxy.setSourceModel(&model);

        QCOMPARE(proxy.rowCount(), 3);

        proxy.setProblemsOnly(true);
        QCOMPARE(proxy.rowCount(), 2); // Mid (giá trị) + Zeta (offset); Alpha sẵn sàng bị ẩn

        // Kết hợp với lọc văn bản: gõ "offset" chỉ còn flag thiếu offset.
        proxy.setFilterFixedString(QStringLiteral("offset"));
        QCOMPARE(proxy.rowCount(), 1);
        QCOMPARE(proxy.index(0, FlagTableModel::NameColumn).data().toString(), QStringLiteral("Zeta"));

        proxy.setFilterFixedString(QString());
        proxy.setProblemsOnly(false);
        proxy.sort(FlagTableModel::StatusColumn, Qt::AscendingOrder);
        // Vấn đề lên trước: MissingOffset (Zeta), InvalidValue (Mid), rồi Ready (Alpha).
        QCOMPARE(proxy.index(0, FlagTableModel::NameColumn).data().toString(), QStringLiteral("Zeta"));
        QCOMPARE(proxy.index(1, FlagTableModel::NameColumn).data().toString(), QStringLiteral("Mid"));
        QCOMPARE(proxy.index(2, FlagTableModel::NameColumn).data().toString(), QStringLiteral("Alpha"));
    }

    void proxyReactsWhenApplyResultsChange()
    {
        FlagStore store;
        store.merge(sampleFlags());
        OffsetMap offsets{{QStringLiteral("Alpha"), 0x10}, {QStringLiteral("Mid"), 0x20},
                          {QStringLiteral("Zeta"), 0x30}};
        FlagTableModel model(&store);
        model.setOffsets(&offsets);
        FlagFilterProxyModel proxy;
        proxy.setSourceModel(&model);
        proxy.setProblemsOnly(true);
        QCOMPARE(proxy.rowCount(), 1); // chỉ Mid (chuỗi)

        QHash<QString, FlagResult> results;
        results.insert(QStringLiteral("Zeta"), {FlagStatus::WriteFailed, QStringLiteral("x")});
        model.setApplyResults(results);

        QCOMPARE(proxy.rowCount(), 2); // Zeta lỗi ghi xuất hiện trong danh sách "vấn đề"
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
