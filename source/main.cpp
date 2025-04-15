#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    qRegisterMetaType<QMap<QString, QString>>("QMap<QString, QString>");
    qRegisterMetaType<QAbstractSocket::SocketState>("QAbstractSocket::SocketState");
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
