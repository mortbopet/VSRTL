#include <QtTest/QTest>

#include "vsrtl_memorytester.h"

class tst_memory : public QObject {
    Q_OBJECT private slots : void functionalTest();
};

void tst_memory::functionalTest() {
    vsrtl::MemoryTester a;

    a.verifyAndInitialize();

    const int n = 1234;
    // Clock the circuit n times
    for (int i = 0; i < n; i++)
        a.clock();

    QVERIFY(a.mem->data_out.template value<uint32_t>() == n / 2);
}

QTEST_APPLESS_MAIN(tst_memory)
#include "tst_memory.moc"
