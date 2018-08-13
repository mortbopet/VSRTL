#include "ripes_alu.h"
#include "ripes_architecture.h"
#include "ripes_constant.h"
#include "ripes_register.h"

#include "ripes_adderandreg.h"
#include "ripes_componentgraphic.h"
#include "ripes_mainwindow.h"

#include <QtTest/QTest>

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
    AdderAndReg design;

    auto components = design.getTopLevelComponents();

    int x = 0;
    for (auto& c : components) {
        ComponentGraphic* i = new ComponentGraphic(c);
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
