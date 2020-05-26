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

// All dynamic components may be imported through op.h
#include "ops/op.h"

using namespace vsrtl::core;

std::unique_ptr<vsrtl::Design> createIncrementByConstantCircuit(int width, int constant) {
    auto design = std::make_unique<vsrtl::Design>("Dynamic design");

    auto add4 = design->create_component<Component>("Add 4");

    // Now we create the internals of the add4 component. We instantiate a signed adder.
    auto adder = add4->create_component<OpAdd<OpType::Signed>>("+", width);
    auto& inputPort = add4->createInputPort("input", width);     // Must be a reference!
    auto& outputPort = add4->createOutputPort("output", width);  // Must be a reference!
    auto c4 = add4->create_component<Constant>("c4", constant, width);

    // Connect the internal structure of the add4 component
    *c4->out >> *adder->op1;
    inputPort >> *adder->op2;
    *adder->out >> outputPort;

    // We create a register and use it to create a sequential circuit
    auto reg = design->create_component<OpReg>("reg", width);
    outputPort >> *reg->in;
    *reg->out >> inputPort;

    return design;
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    Q_INIT_RESOURCE(vsrtl_icons);

    auto dynCircuit = createIncrementByConstantCircuit(8, 4);

    vsrtl::MainWindow w(*dynCircuit);

    w.show();

    app.exec();
}
