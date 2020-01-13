#pragma once

#include <map>
#include <unordered_map>
#include <vector>
#include "../interface/vsrtl.h"

namespace vsrtl {
namespace core {

struct SparseArray {
    void writeMem(VSRTL_VT_U address, VSRTL_VT_U value, int size = sizeof(VSRTL_VT_U)) {
        // writes value from the given address start, and up to $size bytes of
        // $value
        for (int i = 0; i < size; i++) {
            data[address + i] = value & 0xff;
            value >>= 8;
        }
    }

    template <bool byteIndexed = true>
    VSRTL_VT_U readMem(VSRTL_VT_U address, unsigned width = 4) {
        if constexpr (!byteIndexed)
            address <<= 2;

        VSRTL_VT_U value = 0;
        for (unsigned i = 0; i < width; i++)
            value |= data[address++] << (i * CHAR_BIT);

        return value;
    }

    template <bool byteIndexed = true>
    VSRTL_VT_U readMemConst(VSRTL_VT_U address, unsigned width = 4) const {
        if constexpr (!byteIndexed)
            address <<= 2;

        VSRTL_VT_U value = 0;
        for (unsigned i = 0; i < width; i++) {
            value |= contains(address) ? data.at(address) << (i * CHAR_BIT) : 0;
            address++;
        }

        return value;
    }

    bool contains(uint32_t address) const { return data.count(address) > 0; }

    /**
     * @brief addInitializationMemory
     * The specified program will be added as a memory segment which will be loaded into this memory once it is reset.
     */
    template <typename T>
    void addInitializationMemory(const VSRTL_VT_U startAddr, T* program, size_t n) {
        auto& mem = initializationMemories.emplace_back();
        VSRTL_VT_U addr = startAddr;
        for (size_t i = 0; i < n; i++) {
            // Add to initialization memories for future rewriting upon reset
            mem.writeMem(addr, program[i], sizeof(T));
            addr += sizeof(T);
        }
    }

    void clearInitializationMemories() { initializationMemories.clear(); }

    void reset() {
        data.clear();
        for (const auto& mem : initializationMemories) {
            for (const auto& memData : mem.data) {
                writeMem(memData.first, memData.second, sizeof(memData.second));
            }
        }
    }

    std::unordered_map<VSRTL_VT_U, uint8_t> data;
    std::vector<SparseArray> initializationMemories;
};
}  // namespace core
}  // namespace vsrtl
