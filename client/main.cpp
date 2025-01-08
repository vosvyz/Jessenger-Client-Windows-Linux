#include "mainwindow.h"
#include <QApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    qRegisterMetaType<QMap<QString, QString>>();
    qRegisterMetaTypeStreamOperators<QMap<QString, QString>>("QMap<QString, QString>");
    MainWindow w;
    w.show();
    return a.exec();
}
