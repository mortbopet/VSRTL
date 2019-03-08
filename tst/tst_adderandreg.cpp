#include <QtTest/QTest>

#include "vsrtl_adderandreg.h"

class tst_adderAndReg : public QObject {
    Q_OBJECT private slots : void functionalTest();
};

void tst_adderAndReg::functionalTest() {
    vsrtl::AdderAndReg a;

    // Verify that all instantiated objects in the circuit have been connected as they require
    a.verifyAndInitialize();
    std::cout << "value " << std::endl;
    const int n = 10;
    const int expectedValue = n * a.m_cVal;
    // Clock the circuit n times
    for (int i = 0; i < n; i++)
        a.clock();

    // We expect that m_cVal has been added to the register value n times
    // std::cout << "value " << a.reg->out->value<uint32_t>() << std::endl;
    // QVERIFY(a.reg->out->value<uint32_t>() == expectedValue);
}

QTEST_APPLESS_MAIN(tst_adderAndReg)
#include "tst_adderandreg.moc"
