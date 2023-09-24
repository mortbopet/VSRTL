#include <QtTest/QTest>

#include "vsrtl_counter.h"

#include <cmath>

using namespace vsrtl;
using namespace core;

class tst_counter : public QObject {
  Q_OBJECT
private slots:
  void clockTest();
};

template <int n>
void testCounter() {
  vsrtl::core::Counter<n> counter;
  counter.verifyAndInitialize();

  const VSRTL_VT_U powVal = std::pow(2, n);

  QVERIFY(counter.value->out.uValue() == 0);
  counter.clock();
  QVERIFY(counter.value->out.uValue() == 1);
  counter.reset();
  QVERIFY(counter.value->out.uValue() == 0);
  for (unsigned i = 0; i < powVal - 1; i++) {
    counter.clock();
  }
  // Counter should be at max value
  QVERIFY(counter.value->out.uValue() == (powVal - 1));

  // counter should overflow
  counter.clock();
  QVERIFY(counter.value->out.uValue() == 0);
}

void tst_counter::clockTest() {
  testCounter<1>();
  testCounter<4>();
  testCounter<8>();
}
QTEST_APPLESS_MAIN(tst_counter)
#include "tst_counter.moc"
