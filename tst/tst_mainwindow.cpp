#include "vsrtl_alu.h"
#include "vsrtl_architecture.h"
#include "vsrtl_constant.h"
#include "vsrtl_register.h"

#include "vsrtl_adderandreg.h"
#include "vsrtl_componentgraphic.h"
#include "vsrtl_widget.h"

#include <QtTest/QTest>

using namespace vsrtl;

class tst_MainWindow : public QObject {
    Q_OBJECT
private slots:
    void testSimpleArchitecture();
};

void tst_MainWindow::testSimpleArchitecture() {
    int dummy;
    QApplication app(dummy, nullptr);

    AdderAndReg design;
    VSRTLWidget w(design);

    auto components = design.getTopLevelComponents();

    int x = 0;
    for (auto& c : components) {
        ComponentGraphic* i = new ComponentGraphic(*c);
        w.addComponent(i);
        i->initialize();
        i->setPos(QPointF(x, 0));
        x += 150;
    }
    w.show();

    app.exec();
}

QTEST_APPLESS_MAIN(tst_MainWindow)
#include "tst_mainwindow.moc"
