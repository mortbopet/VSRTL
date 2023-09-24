#include <QtTest/QTest>

#include "../interface/vsrtl_binutils.h"
#include "vsrtl_core.h"

namespace vsrtl {
using namespace core;
/**
 * @brief The ContinuousIncrement design
 * A memory is instantiated wherein the following operation is executed:
 *      mem[i] = mem[i-1] + 1;
 */
class ContinuousIncrement : public Design {
public:
  ContinuousIncrement() : Design("Registerfile Tester") {
    mem->setMemory(m_memory);

    idx_reg->out >> mem->addr;
    mem->data_out >> acc_reg->in;
    inc_adder->out >> mem->data_in;
    1 >> idx_adder->op1;
    idx_reg->out >> idx_adder->op2;
    1 >> inc_adder->op1;
    acc_reg->out >> inc_adder->op2;

    wr_en_reg->out >> idx_next_mux->select;
    idx_adder->out >> *idx_next_mux->ins[0];
    idx_reg->out >> *idx_next_mux->ins[1];

    idx_reg->out >> comp->op1;
    (regSize - 1) >> comp->op2;
    idx_next_mux->out >> idx_reg->in;

    // Write/Read state
    wr_en_mux->out >> wr_en_reg->in;
    wr_en_reg->out >> wr_en_mux->select;
    wr_en_reg->out >> mem->wr_en;
    4 >> mem->wr_width;
    0 >> *wr_en_mux->ins[1];
    1 >> *wr_en_mux->ins[0];
  }
  static constexpr unsigned int regSize = 32;

  // Create objects
  SUBCOMPONENT(mem, TYPE(MemoryAsyncRd<regSize, regSize>));

  SUBCOMPONENT(idx_adder, Adder<regSize>);
  SUBCOMPONENT(inc_adder, Adder<regSize>);
  SUBCOMPONENT(wr_en_mux, TYPE(Multiplexer<2, 1>));
  SUBCOMPONENT(idx_next_mux, TYPE(Multiplexer<2, regSize>));

  SUBCOMPONENT(comp, Eq<regSize>);

  SUBCOMPONENT(wr_en_reg, Register<1>);
  SUBCOMPONENT(idx_reg, Register<regSize>);
  SUBCOMPONENT(acc_reg, Register<regSize>);

  ADDRESSSPACEMM(m_memory);
};

class WriteSameIdx : public Design {
public:
  WriteSameIdx() : Design("Registerfile Tester") {
    mem->data_out >> inc_adder->op1;
    1 >> inc_adder->op2;
    inc_adder->out >> mem->data_in;
    1 >> mem->wr_en;
    4 >> mem->wr_width;
    0x0 >> mem->addr;

    mem->setMemory(m_memory);
  }
  static constexpr unsigned int regs = 32;
  static constexpr unsigned int regWidth = CHAR_BIT * sizeof(VSRTL_VT_U);

  // Create objects
  SUBCOMPONENT(mem, TYPE(MemorySyncRd<ceillog2(regs), regWidth>));
  SUBCOMPONENT(inc_adder, Adder<regWidth>);

  ADDRESSSPACEMM(m_memory);
};

} // namespace vsrtl

class tst_memory : public QObject {
  Q_OBJECT;
private slots:

  void repeatedWriteSameIdxSync();
  void functionalTest();
};

void tst_memory::functionalTest() {
  vsrtl::ContinuousIncrement a;

  a.verifyAndInitialize();

  const int n = 1234;
  // Clock the circuit n times
  for (int i = 0; i < n; i++)
    a.clock();

  QVERIFY(a.mem->data_out.uValue() == n / 2);
}

void tst_memory::repeatedWriteSameIdxSync() {
  vsrtl::WriteSameIdx a;

  a.verifyAndInitialize();

  const int n = 10;
  // Clock the circuit n times
  for (unsigned i = 0; i < n; i++) {
    QVERIFY(a.mem->data_out.uValue() == i);
    a.clock();
  }
}

QTEST_APPLESS_MAIN(tst_memory)
#include "tst_memory.moc"
