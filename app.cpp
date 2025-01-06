#include <QApplication>

#include "VSRTL/components/vsrtl_adderandreg.h"
#include "VSRTL/components/vsrtl_counter.h"
#include "VSRTL/components/vsrtl_manynestedcomponents.h"
#include "VSRTL/components/vsrtl_nestedexponenter.h"
#include "VSRTL/components/vsrtl_rannumgen.h"

#include "VSRTL/graphics/vsrtl_mainwindow.h"

#include "VSRTL/components/Leros/SingleCycleLeros/SingleCycleLeros.h"

#include <chrono>

#include <QDebug>
#include <QFile>

int main(int argc, char **argv) {
  QApplication app(argc, argv);

  Q_INIT_RESOURCE(vsrtl_icons);

  vsrtl::AdderAndReg design;

  vsrtl::MainWindow w(design);

  w.show();

  app.exec();
}
