#include "ui/main_window.h"

#include <QApplication>
#include <QIcon>

#ifndef SI_APP_VERSION
#define SI_APP_VERSION "dev"
#endif

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setStyle(QStringLiteral("Fusion"));
    QApplication::setApplicationName(QStringLiteral("Simple Injector"));
    QApplication::setApplicationVersion(QStringLiteral(SI_APP_VERSION));
    QApplication::setOrganizationName(QStringLiteral("SimpleInjector"));
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/icons/app.png")));

    SimpleInjector::Ui::MainWindow window;
    window.show();
    return app.exec();
}
