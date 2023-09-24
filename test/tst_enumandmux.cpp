#include <QtTest/QTest>

#include "vsrtl_enumandmux.h"

class tst_enumAndMux : public QObject {
  Q_OBJECT private slots : void functionalTest();
};

void tst_enumAndMux::functionalTest() { vsrtl::core::EnumAndMux a; }

QTEST_APPLESS_MAIN(tst_enumAndMux)
#include "tst_enumandmux.moc"
