#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    // ~8192 bits (~2460 decimal digits); pick a value that fits your perf profile
    mpf_set_default_prec(8192);

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();

    // This file contains the application's entry point only. 
    // Do not add complex logic here.
}
