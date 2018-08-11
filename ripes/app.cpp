#include <QApplication>
#include "ripes_componentgraphic.h"
#include "ripes_mainwindow.h"
#include "ripes_nestedexponenter.h"

#include <chrono>

#include <QDebug>
#include <QFile>

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    Q_INIT_RESOURCE(icons);

    ripes::MainWindow w;
    ripes::NestedExponenter design;

    auto components = design.getTopLevelComponents();

    int x = 0;
    for (auto& c : components) {
        ripes::ComponentGraphic* i = new ripes::ComponentGraphic(c);
        w.addComponent(i);
        i->initialize();
        i->setPosition(QPointF(x, 0));
        x += 150;
    }
    x = 0;

    w.show();

    app.exec();
}
