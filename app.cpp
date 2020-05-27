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
#include "interface/vsrtl_binutils.h"
#include "ops/op.h"

using namespace vsrtl::core;

std::unique_ptr<vsrtl::Design> createIncrementByConstantCircuit(int resWidth, int constant) {
    auto design = std::make_unique<vsrtl::Design>("Dynamic design");

    auto add4 = design->create_component<Component>("Add Constant");

    // Now we create the internals of the Add Constant component. We instantiate a signed adder.

    // As a test, we try to infer the required width of the constant. The constant will be interpreted as a signed. This
    // will also check that instantiating the adder with different input widths works.
    const unsigned cWidth = vsrtl::bitsToRepresentSValue(constant);
    auto adder = add4->create_component<OpAdd<OpType::Signed>>("+", cWidth, resWidth);
    auto& inputPort = add4->createInputPort("input", resWidth);     // Must be a reference!
    auto& outputPort = add4->createOutputPort("output", resWidth);  // Must be a reference!
    auto c = add4->create_component<Constant>("c", constant, cWidth);
    // Adder will return a 'resWidth' + 1 wide signal. Extract lower 'width' bits (resWidth - 1 downto 0)
    auto bitextr = add4->create_component<OpBitExtr>("extr1", resWidth + 1, 0, resWidth - 1);

    // Connect the internal structure of the add4 component. Operands must be connected in the order which their widths
    // instantiated the adder
    *c->out >> *adder->op1;
    inputPort >> *adder->op2;
    *adder->out >> *bitextr->in;
    *bitextr->out >> outputPort;

    // We create a register and use it to create a sequential circuit
    auto reg = design->create_component<OpReg>("reg", resWidth);
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
