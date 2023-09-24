#include <QtTest/QTest>

#include "vsrtl_rannumgen.h"

using namespace vsrtl;
using namespace core;

class tst_rannumgen : public QObject {
  Q_OBJECT private slots : void functionalTest();
};

VSRTL_VT_U xorshift(VSRTL_VT_U x) {
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  return x;
}

void tst_rannumgen::functionalTest() {
  RanNumGen a;
  a.verifyAndInitialize();

  VSRTL_VT_U seed = 0x13fb27a3;
  // Clock the circuit n times
  for (int i = 0; i < 10; i++) {
    a.clock();
    VSRTL_VT_U circuit_val = a.rngResReg->out.uValue();
    QVERIFY(circuit_val == seed);
    seed = xorshift(seed);
  }
}

QTEST_APPLESS_MAIN(tst_rannumgen)
#include "tst_rannumgen.moc"
