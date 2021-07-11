#include <QApplication>

#include "components/vsrtl_adderandreg.h"
#include "components/vsrtl_counter.h"
#include "components/vsrtl_manynestedcomponents.h"
#include "components/vsrtl_nestedexponenter.h"
#include "components/vsrtl_rannumgen.h"

#include "vsrtl_mainwindow.h"

#include "components/Leros/SingleCycleLeros/SingleCycleLeros.h"

#include "vlt/vlt_design.h"

#include <chrono>

#include <QDebug>
#include <QFile>

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    Q_INIT_RESOURCE(vsrtl_icons);

    // vsrtl::AdderAndReg design;
    vsrtl::vlt::Design design("/home/morten/Downloads/Cores-SweRV/tools/obj_dir/Vtb_top.xml");

    vsrtl::MainWindow w(design);

    w.show();

    app.exec();
}
