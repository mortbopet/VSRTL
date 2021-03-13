#pragma once

#include <assert.h>
#include <functional>
#include <map>
#include <unordered_map>
#include <vector>

#include "../interface/vsrtl.h"

namespace vsrtl {
namespace core {

/**
 * @brief The SparseArray class
 *  A sparse array datastructure used to describe a byte-addressable memory of unbounded size (up to UINT32_MAX keys).
 */
class SparseArray {
public:
    virtual void writeMem(VSRTL_VT_U address, VSRTL_VT_U value, int size = sizeof(VSRTL_VT_U)) {
        // writes value from the given address start, and up to $size bytes of
        // $value
        for (int i = 0; i < size; i++) {
            m_data[address + i] = value & 0xff;
            value >>= 8;
        }
    }

    virtual VSRTL_VT_U readMem(VSRTL_VT_U address, unsigned width = 4) {
        VSRTL_VT_U value = 0;
        for (unsigned i = 0; i < width; i++) {
            value |= m_data[address++] << (i * CHAR_BIT);
        }

        return value;
    }

    virtual VSRTL_VT_U readMemConst(VSRTL_VT_U address, unsigned width = 4) const {
        VSRTL_VT_U value = 0;
        for (unsigned i = 0; i < width; i++) {
            value |= contains(address) ? m_data.at(address) << (i * CHAR_BIT) : 0;
            address++;
        }
        return value;
    }

    virtual bool contains(uint32_t address) const { return m_data.count(address) > 0; }

    /**
     * @brief addInitializationMemory
     * The specified program will be added as a memory segment which will be loaded into this memory once it is reset.
     */
    template <typename T>
    void addInitializationMemory(const VSRTL_VT_U startAddr, T* program, size_t n) {
        auto& mem = m_initializationMemories.emplace_back();
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
        for (const auto& mem : m_initializationMemories) {
            for (const auto& memData : mem.m_data) {
                writeMem(memData.first, memData.second, sizeof(memData.second));
            }
        }
    }

private:
    std::unordered_map<VSRTL_VT_U, uint8_t> m_data;
    std::vector<SparseArray> m_initializationMemories;
};

struct IOFunctors {
    /**
     * @brief ioWrite
     * Function pointer to an IO write function.
     * - @param 1: adress offset relative to the base address of the component
     * - @param 2: value to write
     * - @param 3: byte-width of write
     */
    std::function<void(uint32_t, uint32_t, uint32_t)> ioWrite;
    /**
     * @brief ioRead
     * Function pointer to an IO read function.
     * - @param 1: adress offset relative to the base address of the component
     * - @param 3: byte-width of read
     * - @return read value from peripheral
     */
    std::function<uint32_t(uint32_t, uint32_t)> ioRead;
};

/**
 * @brief The BussedSparseMM class
 * Extends the sparse array with the capabilites of having separate memory regions in the address space wherein
 * read/writes should be forwarded to somewhere else. Used for registerring memory-mapped peripherals.
 */
class SparseArrayMM : public SparseArray {
public:
    virtual void writeMem(VSRTL_VT_U address, VSRTL_VT_U value, int size = sizeof(VSRTL_VT_U)) override {
        if (auto* io = findMMapRegion(address)) {
            io->ioWrite(address, value, size);
        } else {
            SparseArray::writeMem(address, value, size);
        }
    }

    virtual VSRTL_VT_U readMem(VSRTL_VT_U address, unsigned width = 4) override {
        if (auto* io = findMMapRegion(address)) {
            return io->ioRead(address, width);
        } else {
            return SparseArray::readMem(address, width);
        }
    }

    virtual VSRTL_VT_U readMemConst(VSRTL_VT_U address, unsigned width = 4) const override {
        if (auto* io = findMMapRegion(address)) {
            return io->ioRead(address, width);
        } else {
            return SparseArray::readMemConst(address, width);
        }
    }

    /**
     * @brief addRegion
     * Registers a memory mapped region at @param start with @param size.
     */
    void addRegion(VSRTL_VT_U baseAddr, unsigned size, const IOFunctors& io) {
        // Assert that there lies no memory regions within the region that we are about to insert
        assert(findMMapRegion(baseAddr) == nullptr && findMMapRegion(baseAddr + size - 1) == nullptr &&
               "Tried to add memory mapped region which overlaps with some other region");
        m_mmapRegions[baseAddr] = MMapValue{size, io};
    }
    void removeRegion(VSRTL_VT_U baseAddr) {
        auto it = m_mmapRegions.find(baseAddr);
        assert(it != m_mmapRegions.end() && "Tried to remove non-existing memory mapped region");
        m_mmapRegions.erase(it);
    }

    /**
     * @brief findMMapRegion
     * Attempts to locate the memory mapped region which @param address resides in. If located, returns I/O capabilities
     * to this region, else returns nullptr.
     */
    const IOFunctors* findMMapRegion(VSRTL_VT_U address) const {
        if (m_mmapRegions.empty()) {
            return nullptr;
        }

        auto it = m_mmapRegions.lower_bound(address);
        if (it == m_mmapRegions.end()) {
            return nullptr;
        } else if (address >= it->first + it->second.size) {
            return nullptr;
        }
        return &it->second.io;
    }

private:
    struct MMapValue {
        unsigned size;
        IOFunctors io;
    };

    /**
     * @brief m_mmapRegions
     * Map of memory-mapped regions. Key is the base-address of the region. Value represents the size of the region
     * (used to determine indexing into the region) as well as I/O functions.
     */
    std::map<uint32_t, MMapValue> m_mmapRegions;
};

}  // namespace core
}  // namespace vsrtl
