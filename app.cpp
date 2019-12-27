#include <QApplication>

#include "components/vsrtl_adderandreg.h"
#include "components/vsrtl_counter.h"
#include "components/vsrtl_manynestedcomponents.h"
#include "components/vsrtl_nestedexponenter.h"
#include "components/vsrtl_rannumgen.h"

#include "vsrtl_mainwindow.h"

#include "components/Leros/SingleCycleLeros/SingleCycleLeros.h"

#include <chrono>

#include <QDebug>
#include <QFile>

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    Q_INIT_RESOURCE(icons);

    vsrtl::leros::SingleCycleLeros design;

    std::vector<unsigned short> program = {0x2901, 0x3000, 0x5000, 0x2100, 0x7000,
                                           0x6000, 0x0901, 0x7000, 0x2100, 0x8FFC};
    design.instr_mem->addInitializationMemory(0x0, program.data(), program.size());

    vsrtl::MainWindow w(design);

    w.show();

    app.exec();
}
