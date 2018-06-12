#ifndef MEMORY_H
#define MEMORY_H

#include <cstdint>
#include <unordered_map>

#include "ripes_defines.h"
#include "ripes_primitive.h"

namespace ripes {

class Memory : public Primitive<REGISTERWIDTH> {
public:
    Memory() : Primitive("Memory") {}
    void write(uint32_t address, uint32_t value, int size = sizeof(uint32_t));
    uint32_t read(uint32_t address);
    void reset(uint32_t textSize);
    void clear() { m_memory.clear(); }

    void propagate() override {}
    void verifySubtype() const override {}

private:
    std::unordered_map<uint32_t, uint8_t> m_memory;
};
}  // namespace ripes

#endif  // MEMORY_H
