#pragma once

#include <assert.h>
#include <functional>
#include <map>
#include <unordered_map>
#include <vector>

#include "../interface/vsrtl_defines.h"

namespace vsrtl {
namespace core {

/**
 * @brief The AddressSpace class
 * The AddressSpace class manages a sparse array datastructure used to describe
 * a byte-addressable memory of unbounded size (up to UINT32_MAX keys), intended
 * for use as instruction/data memory. Furthermore, initialization memories can
 * be added, which will be re-written to the sparse array upon resetting the
 * memory.
 *
 */
class AddressSpace {
public:
  enum class RegionType { Program, IO };
  virtual ~AddressSpace() {}

  virtual void writeMem(VSRTL_VT_U address, VSRTL_VT_U value, int bytes) {
    // writes value from the given address start, and up to $size bytes of
    // $value
    for (int i = 0; i < bytes; i++) {
      m_data[address++] = value & 0xFF;
      value >>= 8;
    }
  }

  virtual VSRTL_VT_U readMem(VSRTL_VT_U address, unsigned bytes) {
    VSRTL_VT_U value = 0;
    for (unsigned i = 0; i < bytes; i++) {
      value |= static_cast<VSRTL_VT_U>(m_data[address++]) << (i * CHAR_BIT);
    }

    return value;
  }

  virtual VSRTL_VT_U readMemConst(VSRTL_VT_U address, unsigned bytes) const {
    VSRTL_VT_U value = 0;
    for (unsigned i = 0; i < bytes; i++) {
      auto it = m_data.find(address);
      if (it != m_data.end()) {
        value |= static_cast<VSRTL_VT_U>(it->second) << (i * CHAR_BIT);
      }
      address++;
    }
    return value;
  }

  virtual bool contains(const VSRTL_VT_U &address) const {
    return m_data.count(address) > 0;
  }
  virtual RegionType regionType(const VSRTL_VT_U & /* address */) const {
    return RegionType::Program;
  }

  /**
   * @brief addInitializationMemory
   * The specified program will be added as a memory segment which will be
   * loaded into this memory once it is reset.
   */
  template <typename T>
  void addInitializationMemory(const VSRTL_VT_U &startAddr, T *program,
                               const size_t &n) {
    auto &mem = m_initializationMemories.emplace_back();
    VSRTL_VT_U addr = startAddr;
    for (size_t i = 0; i < n; i++) {
      // Add to initialization memories for future rewriting upon reset
      mem.writeMem(addr, program[i], sizeof(T));
      addr += sizeof(T);
    }
  }

  void clearInitializationMemories() { m_initializationMemories.clear(); }

  virtual void reset() {
    m_data.clear();
    for (const auto &mem : m_initializationMemories) {
      for (const auto &memData : mem.m_data) {
        writeMem(memData.first, memData.second, sizeof(memData.second));
      }
    }
  }

private:
  std::unordered_map<VSRTL_VT_U, uint8_t> m_data;
  std::vector<AddressSpace> m_initializationMemories;
};

struct IOFunctors {
  /**
   * @brief ioWrite
   * Function pointer to an IO write function.
   * - @param 1: adress offset relative to the base address of the component
   * - @param 2: value to write
   * - @param 3: byte-width of write
   */
  std::function<void(VSRTL_VT_U, VSRTL_VT_U, VSRTL_VT_U)> ioWrite;
  /**
   * @brief ioRead
   * Function pointer to an IO read function.
   * - @param 1: adress offset relative to the base address of the component
   * - @param 3: byte-width of read
   * - @return read value from peripheral
   */
  std::function<VSRTL_VT_U(VSRTL_VT_U, VSRTL_VT_U)> ioRead;
};

/**
 * @brief The AddressSpaceMM class
 * Extends the AddressSpace with the capabilites of having separate memory
 * regions in the address space wherein read/writes should be forwarded to
 * somewhere else. Used for registerring memory-mapped peripherals.
 */
class AddressSpaceMM : public AddressSpace {
public:
  struct MMapValue {
    VSRTL_VT_U base;
    unsigned size;
    IOFunctors io;
  };

  virtual void writeMem(VSRTL_VT_U address, VSRTL_VT_U value,
                        int size = sizeof(VSRTL_VT_U)) override {
    if (auto *mmapregion = findMMapRegion(address)) {
      mmapregion->io.ioWrite(address - mmapregion->base, value, size);
    } else {
      AddressSpace::writeMem(address, value, size);
    }
  }

  virtual VSRTL_VT_U readMem(VSRTL_VT_U address, unsigned width) override {
    if (auto *mmapregion = findMMapRegion(address)) {
      return mmapregion->io.ioRead(address - mmapregion->base, width);
    } else {
      return AddressSpace::readMem(address, width);
    }
  }

  virtual VSRTL_VT_U readMemConst(VSRTL_VT_U address,
                                  unsigned width) const override {
    if (auto *mmapregion = findMMapRegion(address)) {
      return mmapregion->io.ioRead(address - mmapregion->base, width);
    } else {
      return AddressSpace::readMemConst(address, width);
    }
  }

  RegionType regionType(const VSRTL_VT_U &address) const override {
    if (auto *mmapregion = findMMapRegion(address)) {
      (void)mmapregion;
      return RegionType::IO;
    } else {
      return RegionType::Program;
    }
  }

  /**
   * @brief addRegion
   * Registers a memory mapped region at @param start with @param size.
   */
  void addIORegion(const VSRTL_VT_U &baseAddr, const unsigned &size,
                   const IOFunctors &io) {
    // Assert that there lies no memory regions within the region that we are
    // about to insert
    assert(findMMapRegion(baseAddr) == nullptr &&
           findMMapRegion(baseAddr + size - 1) == nullptr &&
           "Tried to add memory mapped region which overlaps with some other "
           "region");
    assert(size > 0);
    m_mmapRegions[baseAddr + size - 1] = MMapValue{baseAddr, size, io};
  }
  void removeIORegion(const VSRTL_VT_U &baseAddr, const unsigned &size) {
    auto it = m_mmapRegions.find(baseAddr + size - 1);
    assert(it != m_mmapRegions.end() &&
           "Tried to remove non-existing memory mapped region");
    m_mmapRegions.erase(it);
  }

  /**
   * @brief findMMapRegion
   * Attempts to locate the memory mapped region which @param address resides
   * in. If located, returns I/O capabilities to this region, else returns
   * nullptr.
   */
  const MMapValue *findMMapRegion(const VSRTL_VT_U &address) const {
    if (m_mmapRegions.empty()) {
      return nullptr;
    }

    auto it = m_mmapRegions.lower_bound(address);
    if (it == m_mmapRegions.end()) {
      return nullptr;
    } else if (address < it->second.base) {
      return nullptr;
    }

    return &it->second;
  }

private:
  /**
   * @brief m_mmapRegions
   * Map of memory-mapped regions. Key is the last address of the region. Might
   * seem like a weird choice (instead of base address of the region), however,
   * this logic works better with std::map::lower_bound. Value represents the
   * size of the region (used to determine indexing into the region) as well as
   * I/O functions.
   */
  std::map<VSRTL_VT_U, MMapValue> m_mmapRegions;
};

} // namespace core
} // namespace vsrtl
