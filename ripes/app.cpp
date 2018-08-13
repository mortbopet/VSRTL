#include <QApplication>
#include "ripes_adderandreg.h"
#include "ripes_mainwindow.h"
#include "ripes_nestedexponenter.h"

#include <chrono>

#include <QDebug>
#include <QFile>

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    Q_INIT_RESOURCE(icons);

    ripes::MainWindow w;
    ripes::AdderAndReg design;

    w.initializeArchitecture(&design);

    w.show();

    app.exec();
}
