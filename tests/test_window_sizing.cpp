#include "ui/window_sizing.h"

#include <QtTest>

using namespace SimpleInjector;

class WindowSizingTests final : public QObject {
    Q_OBJECT

private slots:
    void keepsPreferredSizeOnLargeScreen()
    {
        QCOMPARE(fitToScreen({620, 720}, QRect(0, 0, 1920, 1040)), QSize(620, 720));
    }

    void shrinksToFitSmallLaptopScreen()
    {
        // 1366x768 minus the taskbar ~ 1366x728: a height of 720 would overflow, so shrink to 90%.
        const QSize fitted = fitToScreen({620, 720}, QRect(0, 0, 1366, 728));
        QCOMPARE(fitted, QSize(620, 655));
        QVERIFY(fitted.height() < 728);
    }

    void shrinksBothDimensionsOnTinyScreen()
    {
        QCOMPARE(fitToScreen({620, 720}, QRect(0, 0, 400, 500)), QSize(360, 450));
    }

    void neverReturnsZeroForDegenerateScreen()
    {
        const QSize fitted = fitToScreen({620, 720}, QRect(0, 0, 0, 0));
        QVERIFY(fitted.width() >= 1 && fitted.height() >= 1);
    }

    void minimumSizeAlsoClampsOnSmallScreens()
    {
        QCOMPARE(fitToScreen({520, 480}, QRect(0, 0, 800, 400)), QSize(520, 360));
    }

    void centersInsideAvailableAreaWithOffset()
    {
        // Taskbar at the top: the available area starts at y = 40.
        const QRect available(0, 40, 1000, 700);
        const QRect centered = centeredIn({600, 500}, available);
        QCOMPARE(centered, QRect(200, 140, 600, 500));
        QVERIFY(available.contains(centered));
    }
};

QTEST_GUILESS_MAIN(WindowSizingTests)
#include "test_window_sizing.moc"
