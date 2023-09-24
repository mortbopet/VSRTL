#include <QtTest/QTest>

#include "Leros/SingleCycleLeros/SingleCycleLeros.h"

class tst_Leros : public QObject {
  Q_OBJECT

private slots:
  void functionalTest();
  void incInAccumulator();
  void incInRegister();
  void incInMemory();
  void startupInc();
};

void tst_Leros::startupInc() {
  std::vector<unsigned short> program = {
      0x2100, 0x3064, 0x2101, 0x3065, 0x2180, 0x2900, 0x3066, 0x2100, 0x2980,
      0x2a00, 0x3067, 0x2100, 0x2b80, 0x3068, 0x21ff, 0x2900, 0x3069, 0x29ff,
      0x2a00, 0x306a, 0x2100, 0x2aff, 0x306b, 0x21ff, 0x2b7f, 0x306c, 0x21cc,
      0x2900, 0x2a00, 0x2b00, 0x3078, 0x21e4, 0x2900, 0x2a00, 0x2b00, 0x3079,
      0x21fc, 0x2900, 0x2a00, 0x2b00, 0x307a, 0x2100, 0x2900, 0x2a00, 0x2b00,
      0x9011, 0x3001, 0x2100, 0x2900, 0x2a00, 0x2b20, 0x3002, 0x5002, 0x2100,
      0x7000, 0x2002, 0x0904, 0x3002, 0x2001, 0x0d01, 0x3001, 0xaff7, 0x21fc,
      0x290f, 0x2a00, 0x2b20, 0x3001, 0x2194, 0x2900, 0x2a00, 0x2b00, 0x4000,
      0x8000, 0x0000, 0x2001, 0x09f0, 0x3001, 0x2000, 0x5001, 0x7003, 0x2002,
      0x7002, 0x2001, 0x0910, 0x3002, 0x8001, 0x5002, 0x60fd, 0x0901, 0x70fd,
      0x60fd, 0x2304, 0x3004, 0x9008, 0x8001, 0x5002, 0x60fc, 0x0901, 0x3004,
      0x70fc, 0x8001, 0x8ff1, 0x2005, 0x9009, 0x2004, 0x0804, 0x3004, 0x2005,
      0x0d01, 0x9003, 0x3005, 0x8ff9, 0x2000, 0x4000, 0x2005, 0x9009, 0x2004,
      0x1000, 0x3004, 0x2005, 0x0d01, 0x9003, 0x3005, 0x8ff9, 0x2000, 0x4000,
      0x2005, 0x900a, 0x2004, 0x1000, 0x226c, 0x3004, 0x2005, 0x0d01, 0x9003,
      0x3005, 0x8ff8, 0x2000, 0x4000};
  Q_UNUSED(program);
}

void tst_Leros::functionalTest() { vsrtl::leros::SingleCycleLeros design; }

void tst_Leros::incInRegister() {
  vsrtl::leros::SingleCycleLeros design;

  /**
   * load     0
   * addi     1
   * store    0
   * br       -6
   */
  std::vector<unsigned short> program = {0x2000, 0x0901, 0x3000, 0x8FFD};
  design.m_memory->addInitializationMemory(0x0, program.data(), program.size());
  design.verifyAndInitialize();
}
void tst_Leros::incInMemory() {
  vsrtl::leros::SingleCycleLeros design;

  /**
          loadhi  1   -- 0x100
          store   0
          ldaddr  0
          loadi   0
          stind   0   -- store 0 at 0x100[0]
  .loop:
          ldind   0
          addi    1
          stind   0
          loadi   0   -- ensure that value is not staying in accummulator
          br      -8
   */
  std::vector<unsigned short> program = {0x2901, 0x3000, 0x5000, 0x2100,
                                         0x7000, 0x6000, 0x0901, 0x7000,
                                         0x2100, 0x8FFC};

  design.m_memory->addInitializationMemory(0x0, program.data(), program.size());
  design.verifyAndInitialize();

  constexpr int accTarget = 10;

  // Go to .loop
  for (int i = 0; i < 5; i++)
    design.clock();

  // Increment
  for (unsigned int i = 0; i < accTarget; i++) {
    design.clock(); // Execute ldind 0
    QVERIFY(design.acc_reg->out.uValue() == i);
    design.clock();
    design.clock();
    design.clock();
    design.clock();
  }

  // Reverse
  for (unsigned int i = accTarget - 1; i >= accTarget / 2; i--) {
    design.reverse();
    design.reverse();
    design.reverse();
    design.reverse();
    QCOMPARE(design.acc_reg->out.uValue(), i);
    design.reverse();
  }
}

void tst_Leros::incInAccumulator() {
  vsrtl::leros::SingleCycleLeros design;

  /**
   * addi 1
   * br -2
   */
  std::vector<unsigned short> program = {0x0901, 0x8FFF};
  design.m_memory->addInitializationMemory(0x0, program.data(), program.size());
  design.verifyAndInitialize();

  constexpr int accTarget = 10;
  QVERIFY(design.acc_reg->out.uValue() == 0);

  // Increment
  for (uint32_t i = 0; i < accTarget; i++) {
    QVERIFY(design.acc_reg->out.uValue() == i);
    // cycle 1: addi, cycle 2: br
    design.clock();
    design.clock();
  }

  // Reverse
  for (uint32_t i = accTarget; i >= accTarget / 2; i--) {
    QVERIFY(design.acc_reg->out.uValue() == i);
    design.reverse();
    design.reverse();
  }
}

QTEST_APPLESS_MAIN(tst_Leros)
#include "tst_leros.moc"
