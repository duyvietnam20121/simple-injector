#include "platform/process_monitor.h"

#include <QQueue>
#include <QSignalSpy>
#include <QtTest>

using namespace SimpleInjector;

namespace {

/// Probe giả: trả lần lượt các trạng thái đã xếp hàng; hết hàng thì lặp lại trạng thái cuối.
class FakeProbe final : public IProcessProbe {
public:
    ProcessStatus refresh() override
    {
        ++refreshCalls;
        if (!script.isEmpty())
            last = script.dequeue();
        return last;
    }
    void detach() override { ++detachCalls; }

    void push(const ProcessStatus &status) { script.enqueue(status); }

    QQueue<ProcessStatus> script;
    ProcessStatus last;
    int refreshCalls = 0;
    int detachCalls = 0;
};

ProcessStatus notRunning() { return {ProcessState::NotRunning, 0, 0, 0, QString()}; }
ProcessStatus denied() { return {ProcessState::AccessDenied, 10, 0, 5, QString()}; }
ProcessStatus connected(quint32 pid, const QString &version = QStringLiteral("version-a"))
{
    return {ProcessState::Connected, pid, 0x140000000, 0, version};
}

} // namespace

class ProcessMonitorTests final : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() { qRegisterMetaType<SimpleInjector::ProcessStatus>(); }

    void firstCheckAlwaysEmitsStatus()
    {
        FakeProbe probe;
        probe.push(notRunning());
        ProcessMonitor monitor(&probe);
        QSignalSpy changed(&monitor, &ProcessMonitor::statusChanged);
        QSignalSpy attached(&monitor, &ProcessMonitor::attached);

        monitor.checkNow();

        QCOMPARE(changed.count(), 1);
        QCOMPARE(attached.count(), 0);
        QVERIFY(!monitor.isConnected());
    }

    void unchangedStatusEmitsNothing()
    {
        FakeProbe probe;
        probe.push(denied());
        ProcessMonitor monitor(&probe);
        monitor.checkNow();

        QSignalSpy changed(&monitor, &ProcessMonitor::statusChanged);
        monitor.checkNow();
        monitor.checkNow();

        QCOMPARE(changed.count(), 0);
        QCOMPARE(probe.refreshCalls, 3);
    }

    void attachedFiresOncePerConnection()
    {
        FakeProbe probe;
        probe.push(notRunning());
        probe.push(connected(100));
        ProcessMonitor monitor(&probe);
        QSignalSpy attached(&monitor, &ProcessMonitor::attached);

        monitor.checkNow(); // not running
        monitor.checkNow(); // connected
        monitor.checkNow(); // vẫn connected, cùng trạng thái
        monitor.checkNow();

        QCOMPARE(attached.count(), 1);
        QVERIFY(monitor.isConnected());
        QCOMPARE(monitor.status().pid, quint32(100));
    }

    void reconnectAfterDisconnectFiresAgain()
    {
        FakeProbe probe;
        probe.push(connected(100));
        probe.push(notRunning());
        probe.push(connected(101));
        ProcessMonitor monitor(&probe);
        QSignalSpy attached(&monitor, &ProcessMonitor::attached);

        monitor.checkNow();
        monitor.checkNow();
        monitor.checkNow();

        QCOMPARE(attached.count(), 2);
    }

    void pidChangeWhileConnectedCountsAsNewAttach()
    {
        FakeProbe probe;
        probe.push(connected(100));
        probe.push(connected(200));
        ProcessMonitor monitor(&probe);
        QSignalSpy attached(&monitor, &ProcessMonitor::attached);
        QSignalSpy changed(&monitor, &ProcessMonitor::statusChanged);

        monitor.checkNow();
        monitor.checkNow();

        QCOMPARE(attached.count(), 2);
        QCOMPARE(changed.count(), 2);
    }

    void versionChangeEmitsStatusButNotAttach()
    {
        FakeProbe probe;
        probe.push(connected(100, QStringLiteral("version-a")));
        probe.push(connected(100, QStringLiteral("version-b")));
        ProcessMonitor monitor(&probe);
        QSignalSpy attached(&monitor, &ProcessMonitor::attached);
        QSignalSpy changed(&monitor, &ProcessMonitor::statusChanged);

        monitor.checkNow();
        monitor.checkNow();

        QCOMPARE(changed.count(), 2);
        QCOMPARE(attached.count(), 1);
        QCOMPARE(monitor.status().version, QStringLiteral("version-b"));
    }

    void statusPayloadCarriesDetails()
    {
        FakeProbe probe;
        probe.push(denied());
        ProcessMonitor monitor(&probe);
        QSignalSpy changed(&monitor, &ProcessMonitor::statusChanged);

        monitor.checkNow();

        const auto status = changed.at(0).at(0).value<ProcessStatus>();
        QCOMPARE(status.state, ProcessState::AccessDenied);
        QCOMPARE(status.errorCode, quint32(5));
    }

    void startPollsPeriodicallyUntilStopped()
    {
        FakeProbe probe;
        probe.push(notRunning());
        ProcessMonitor monitor(&probe, 20);

        monitor.start();
        QCOMPARE(probe.refreshCalls, 1); // kiểm tra ngay khi start
        QTRY_VERIFY_WITH_TIMEOUT(probe.refreshCalls >= 4, 3000);

        monitor.stop();
        QTest::qWait(60);
        const int settled = probe.refreshCalls;
        QTest::qWait(100);
        QCOMPARE(probe.refreshCalls, settled);
    }
};

QTEST_GUILESS_MAIN(ProcessMonitorTests)
#include "test_process_monitor.moc"
