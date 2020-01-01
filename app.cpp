#include <QApplication>

#include "components/vsrtl_adderandreg.h"
#include "components/vsrtl_counter.h"
#include "components/vsrtl_manynestedcomponents.h"
#include "components/vsrtl_nestedexponenter.h"
#include "components/vsrtl_rannumgen.h"

#include "vsrtl_mainwindow.h"

#include "components/Leros/SingleCycleLeros/SingleCycleLeros.h"
#include "components/RISC-V/riscv_ss.h"

#include <chrono>

#include <QDebug>
#include <QFile>

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    Q_INIT_RESOURCE(icons);

    vsrtl::RISCV::SingleCycleRISCV design;

    /**
    start:
    li a0 0
    li a1 10
    loop:
    addi a0 a0 1
    bne a0 a1 loop
    j start
    */
    std::vector<unsigned> program = {0x00000513, 0x00a00593, 0x00150513, 0xfeb51ee3, 0xff1ff06f};
    design.instr_mem->addInitializationMemory(0x0, program.data(), program.size());

    vsrtl::MainWindow w(design);

    w.show();

    app.exec();
}
