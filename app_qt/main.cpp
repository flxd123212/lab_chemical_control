/**
 * 实验室危化品库房环境智能管控系统 (Qt版本)
 * main.cpp - 应用程序入口
 */
#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("Lab Chemical Control");
    a.setApplicationVersion("1.0");

    MainWindow w;
    w.showFullScreen();     // GEC6818 LCD 全屏 800x480
    // w.show();            // 桌面调试时取消注释这行

    return a.exec();
}
