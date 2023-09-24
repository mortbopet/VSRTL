#include <QtTest/QTest>

#include "vsrtl_nestedexponenter.h"

class tst_NestedComponents : public QObject {
  Q_OBJECT private slots : void functionalTest();
};

void tst_NestedComponents::functionalTest() {
  vsrtl::core::NestedExponenter a;

  // Verify that all instantiated objects in the circuit have been connected as
  // they require
  a.verifyAndInitialize();

  const int n = 10;
  // Clock the circuit n times
  for (int i = 0; i < n; i++)
    a.clock();

  // We expect that m_cVal has been added to the register value n times
  // REQUIRE(a.regs->value(5) == 40);
}
QTEST_APPLESS_MAIN(tst_NestedComponents)
#include "tst_nestedcomponent.moc"
