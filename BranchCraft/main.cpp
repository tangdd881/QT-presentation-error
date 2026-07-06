#include "MainWindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("YourCompany");
    QCoreApplication::setApplicationName("BranchCraft");
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}