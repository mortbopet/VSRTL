#ifndef MEMORY_H
#define MEMORY_H

#include <cstdint>
#include <unordered_map>

namespace vsrtl {

class Memory {
public:
    Memory();
    void write(uint32_t address, uint32_t value, int size = sizeof(uint32_t));
    uint32_t read(uint32_t address);
    void reset(uint32_t textSize);
    void clear() { m_memory.clear(); }

private:
    std::unordered_map<uint32_t, uint8_t> m_memory;
};
}  // namespace vsrtl

#endif  // MEMORY_H
