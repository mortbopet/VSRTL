#include "ripes_mainwindow.h"

#include "tst_util.h"

#include "catch.hpp"

#include <QApplication>

TEST_CASE("Test main window") {
    int dummy;
    QApplication app(dummy, nullptr);

    Ripes_MainWindow w;

    w.show();

    app.exec();
}
