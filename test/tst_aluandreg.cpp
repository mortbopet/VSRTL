#include <QtTest/QTest>

#include "vsrtl_aluandreg.h"

class tst_ALUAndReg : public QObject {
  Q_OBJECT private slots : void functionalTest();
};

void tst_ALUAndReg::functionalTest() {
  vsrtl::core::ALUAndReg a;

  // Verify that all instantiated objects in the circuit have been connected as
  // they require
  a.verifyAndInitialize();
  std::cout << "value " << std::endl;
  const int n = 10;
  const int expectedValue = n * a.m_cVal;
  // Clock the circuit n times
  for (int i = 0; i < n; i++)
    a.clock();

  // We expect that m_cVal has been added to the register value n times
  QVERIFY(a.reg->out.uValue() == expectedValue);
}

QTEST_APPLESS_MAIN(tst_ALUAndReg)
#include "tst_aluandreg.moc"
