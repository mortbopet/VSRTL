#ifndef MEMORY_H
#define MEMORY_H

#include "vsrtl_addressspace.h"
#include "vsrtl_component.h"
#include "vsrtl_register.h"

#include "../interface/vsrtl_defines.h"
#include "../interface/vsrtl_gfxobjecttypes.h"

#include <cstdint>
#include <unordered_map>

namespace vsrtl {
namespace core {

// A memory eviction contains information about data that was overwritten in a
// given cycle.
struct MemoryEviction {
  long long cycle;
  VSRTL_VT_U addr;
  VSRTL_VT_U data;
  VSRTL_VT_U width;
};

template <bool byteIndexed = true>
class BaseMemory {
public:
  BaseMemory() {}

  void setMemory(AddressSpace *mem) { m_memory = mem; }
  const AddressSpace *memory() const { return m_memory; }
  virtual AddressSpace::RegionType accessRegion() const = 0;

  VSRTL_VT_U read(VSRTL_VT_U address, int size, unsigned wordShift) {
    return m_memory->readMem(byteIndexed ? address : address << wordShift,
                             size);
  }

  void write(VSRTL_VT_U address, VSRTL_VT_U value, int size,
             unsigned wordShift) {
    m_memory->writeMem(byteIndexed ? address : address << wordShift, value,
                       size);
  }

  // Width-independent accessors to memory in- and output signals.
  virtual VSRTL_VT_U addressSig() const = 0;
  virtual VSRTL_VT_U wrEnSig() const = 0;
  virtual VSRTL_VT_U opSig() const { return 0; };

protected:
  AddressSpace *m_memory = nullptr;
};

template <unsigned int addrWidth, unsigned int dataWidth,
          bool byteIndexed = true>
class WrMemory : public ClockedComponent, public BaseMemory<byteIndexed> {
public:
  SetGraphicsType(Component);
  WrMemory(const std::string &name, SimComponent *parent)
      : ClockedComponent(name, parent) {}
  void reset() override { m_reverseStack.clear(); }
  AddressSpace::RegionType accessRegion() const override {
    return this->memory()->regionType(addr.uValue());
  }

  void save() override {
    constexpr unsigned wordshift =
        ceillog2((byteIndexed ? addrWidth : dataWidth) / CHAR_BIT);
    const bool writeEnable = static_cast<bool>(wr_en);
    if (writeEnable) {
      const VSRTL_VT_U addr_v = addr.uValue();
      const VSRTL_VT_U data_in_v = data_in.uValue();
      const VSRTL_VT_U data_out_v =
          this->read(addr_v, dataWidth / CHAR_BIT, wordshift);
      const VSRTL_VT_U wr_width_v = wr_width.uValue();
      // save() is called prior to cycle incrementation; WrMemory::reverse()
      // relies on an eviction being listed for the cycle which the 'reverse'
      // call happened in.´
      const auto cycle = getDesign()->getCycleCount() + 1;
      auto ev = MemoryEviction({cycle, addr_v, data_out_v, wr_width_v});
      saveToStack(ev);
      this->write(addr_v, data_in_v, wr_width_v, wordshift);
    }
  }

  void reverse() override {
    if (m_reverseStack.size() > 0) {
      auto lastEviction = m_reverseStack.front();
      auto cycle = getDesign()->getCycleCount();
      if (lastEviction.cycle == cycle) {
        this->write(lastEviction.addr, lastEviction.data, lastEviction.width,
                    ceillog2((byteIndexed ? addrWidth : dataWidth) / CHAR_BIT));
        m_reverseStack.pop_front();
      }
    }
  }

  virtual VSRTL_VT_U addressSig() const override { return addr.uValue(); };
  virtual VSRTL_VT_U wrEnSig() const override { return wr_en.uValue(); };

  void forceValue(VSRTL_VT_U address, VSRTL_VT_U value) override {
    this->write(address, value, dataWidth / CHAR_BIT,
                ceillog2((byteIndexed ? addrWidth : dataWidth) / CHAR_BIT));
  }

  INPUTPORT(addr, addrWidth);
  INPUTPORT(data_in, dataWidth);
  INPUTPORT(wr_width, ceillog2(dataWidth / CHAR_BIT + 1)); // # bytes
  INPUTPORT(wr_en, 1);

protected:
  void reverseStackSizeChanged() override {
    if (reverseStackSize() < m_reverseStack.size()) {
      m_reverseStack.resize(m_reverseStack.size());
    }
  }

private:
  void saveToStack(MemoryEviction v) {
    m_reverseStack.push_front(v);
    if (m_reverseStack.size() > reverseStackSize()) {
      m_reverseStack.pop_back();
    }
  }

  std::deque<MemoryEviction> m_reverseStack;
};

template <unsigned int addrWidth, unsigned int dataWidth,
          bool byteIndexed = true>
class MemorySyncRd : public WrMemory<addrWidth, dataWidth, byteIndexed> {
public:
  MemorySyncRd(const std::string &name, SimComponent *parent)
      : WrMemory<addrWidth, dataWidth, byteIndexed>(name, parent) {
    data_out << [=] {
      return this->read(
          this->addr.uValue(), dataWidth / CHAR_BIT,
          ceillog2((byteIndexed ? addrWidth : dataWidth) / CHAR_BIT));
    };
  }

  OUTPUTPORT(data_out, dataWidth);

private:
  std::unordered_map<VSRTL_VT_U, uint8_t> m_memory;
};

template <unsigned int addrWidth, unsigned int dataWidth,
          bool byteIndexed = true>
class RdMemory : public Component, public BaseMemory<byteIndexed> {
  template <unsigned int, unsigned int>
  friend class Memory;

public:
  SetGraphicsType(ClockedComponent);
  RdMemory(const std::string &name, SimComponent *parent)
      : Component(name, parent) {
    data_out << [=] {
      auto _addr = addr.uValue();
      auto val = this->read(
          _addr, dataWidth / CHAR_BIT,
          ceillog2((byteIndexed ? addrWidth : dataWidth) / CHAR_BIT));
      return val;
    };
  }

  AddressSpace::RegionType accessRegion() const override {
    return this->memory()->regionType(addr.uValue());
  }

  virtual VSRTL_VT_U addressSig() const override { return addr.uValue(); };
  virtual VSRTL_VT_U wrEnSig() const override { return 0; };

  INPUTPORT(addr, addrWidth);
  OUTPUTPORT(data_out, dataWidth);
};

template <unsigned int addrWidth, unsigned int dataWidth,
          bool byteIndexed = true>
class MemoryAsyncRd : public Component {
public:
  SetGraphicsType(ClockedComponent);
  MemoryAsyncRd(const std::string &name, SimComponent *parent)
      : Component(name, parent) {
    addr >> _wr_mem->addr;
    wr_en >> _wr_mem->wr_en;
    data_in >> _wr_mem->data_in;
    wr_width >> _wr_mem->wr_width;

    addr >> _rd_mem->addr;
    _rd_mem->data_out >> data_out;
  }

  void setMemory(AddressSpace *mem) {
    _wr_mem->setMemory(mem);
    _rd_mem->setMemory(mem);
  }

  AddressSpace::RegionType accessRegion() const {
    return this->memory()->regionType(addr.uValue());
  }

  const AddressSpace *memory() const { return _wr_mem->memory(); }

  SUBCOMPONENT(_rd_mem, TYPE(RdMemory<addrWidth, dataWidth, byteIndexed>));
  SUBCOMPONENT(_wr_mem, TYPE(WrMemory<addrWidth, dataWidth, byteIndexed>));

  INPUTPORT(addr, addrWidth);
  INPUTPORT(data_in, dataWidth);
  INPUTPORT(wr_en, 1);
  INPUTPORT(wr_width, ceillog2(dataWidth / CHAR_BIT + 1)); // # bytes
  OUTPUTPORT(data_out, dataWidth);
};

template <unsigned int addrWidth, unsigned int dataWidth,
          bool byteIndexed = true>
class ROM : public RdMemory<addrWidth, dataWidth, byteIndexed> {
public:
  ROM(const std::string &name, SimComponent *parent)
      : RdMemory<addrWidth, dataWidth, byteIndexed>(name, parent) {}
};

} // namespace core
} // namespace vsrtl

#endif // MEMORY_H
