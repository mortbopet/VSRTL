#include <QtTest/QTest>

#include "RISC-V/riscv_ss.h"

class tst_RISCV : public QObject {
    Q_OBJECT

private slots:
    void regIncBrJump();
};

void tst_RISCV::regIncBrJump() {
    /**
  start:
    li a0 0
    li a1 10
  loop:
    addi a0 a0 1
    bne a0 a1 loop
    j start
    */
    std::vector<unsigned> program = {0x00000513, 0x00a00593, 0x00150513, 0xfeb51ee3, 0xff1ff06f};
    vsrtl::RISCV::SingleCycleRISCV design;

    design.instr_mem->addInitializationMemory(0x0, program.data(), program.size());
    design.verifyAndInitialize();

    int cycles = 0;
    int jumpCount = 0;
    const int jumpTarget = 3;
    const int maxCycles = 100;
    const int jumpInstrAddr = 16;
    while (cycles < maxCycles) {
        if (design.pc_reg->getOut()->uValue() == jumpInstrAddr) {
            jumpCount++;
        }
        if (jumpCount == jumpTarget)
            break;

        design.clock();
        cycles++;
        if (cycles >= maxCycles) {
            QFAIL("Maximum cycle count reached");
        }
    }
}

QTEST_APPLESS_MAIN(tst_RISCV)
#include "tst_riscv.moc"
