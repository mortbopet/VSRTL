#include <QtTest/QTest>

#include "vsrtl_registerfilecmp.h"

class tst_registerfile : public QObject {
  Q_OBJECT private slots : void functionalTest();
};

void tst_registerfile::functionalTest() {
  vsrtl::core::RegisterFileTester a;

  a.verifyAndInitialize();
}

QTEST_APPLESS_MAIN(tst_registerfile)
#include "tst_registerfile.moc"
