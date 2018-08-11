#include "ripes_alu.h"
#include "ripes_architecture.h"
#include "ripes_constant.h"
#include "ripes_register.h"

#include "ripes_componentgraphic.h"
#include "ripes_mainwindow.h"

#include <QtTest/QTest>

namespace ripes {

class adderAndReg : public Architecture<3> {
public:
    static constexpr int m_cVal = 4;

    // Create objects
    SUBCOMPONENT(alu_ctrl, Constant, ALUctrlWidth(), ALU_OPCODE::ADD);
    SUBCOMPONENT(c4, Constant, 32, 4);
    SUBCOMPONENT(alu, ALU, 32);
    SUBCOMPONENT(reg, Register, 32);

    adderAndReg() : Architecture() {
        // Connect objects
        connectSignal(c4->out, alu->op1);
        connectSignal(reg->out, alu->op2);
        connectSignal(alu_ctrl->out, alu->ctrl);
        connectSignal(alu->out, reg->in);
    }
};
}  // namespace ripes

using namespace ripes;

class tst_MainWindow : public QObject {
    Q_OBJECT
private slots:
    void testSimpleArchitecture();
};

void tst_MainWindow::testSimpleArchitecture() {
    int dummy;
    QApplication app(dummy, nullptr);

    MainWindow w;
    adderAndReg design;

    auto components = design.getTopLevelComponents();

    int x = 0;
    for (auto& c : components) {
        ComponentGraphic* i = new ComponentGraphic(c);
        w.addComponent(i);
        i->initialize();
        i->setPosition(QPointF(x, 0));
        x += 150;
    }
    w.show();

    app.exec();
}

QTEST_APPLESS_MAIN(tst_MainWindow)
#include "tst_mainwindow.moc"
