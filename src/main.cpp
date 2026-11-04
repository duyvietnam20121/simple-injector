#include "ui/main_window.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setStyle(QStringLiteral("Fusion"));
    QApplication::setApplicationName(QStringLiteral("Simple Injector"));
    QApplication::setOrganizationName(QStringLiteral("SimpleInjector"));

    SimpleInjector::Ui::MainWindow window;
    window.show();
    return app.exec();
}
