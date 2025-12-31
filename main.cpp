#include "mainwindow.h"
#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 设置应用程序样式
    a.setStyle(QStyleFactory::create("Fusion"));

    // 设置应用程序信息
    QApplication::setApplicationName("学生成绩与分析系统");
    QApplication::setOrganizationName("Qt School");

    MainWindow w;
    w.show();

    return a.exec();
}

