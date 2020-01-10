#pragma once

#include <map>
#include <unordered_map>
#include <vector>
#include "../interface/vsrtl.h"

namespace vsrtl {
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
    VSRTL_VT_U readMem(VSRTL_VT_U address) {
        // Note: If address is not found in memory map, a default constructed object
        // will be created, and read. in our case uint8_t() = 0
        if constexpr (byteIndexed) {
            return (data[address] | (data[address + 1] << 8) | (data[address + 2] << 16) | (data[address + 3] << 24));
        } else {
            return (data[(address << 2)] | (data[(address << 2) + 1] << 8) | (data[(address << 2) + 2] << 16) |
                    (data[(address << 2) + 3] << 24));
        }
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
}  // namespace vsrtl
