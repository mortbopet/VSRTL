#include "vsrtl_memory.h"

namespace vsrtl {

void Memory::write(uint32_t address, uint32_t value, int size) {
    // writes value from the given address start, and up to $size bytes of
    // $value
    for (int i = 0; i < size; i++) {
        m_memory[address + i] = value & 0xff;
        value >>= 8;
    }
}
uint32_t Memory::read(uint32_t address) {
    // Note: If address is not found in memory map, a default constructed object
    // will be created, and read. in our case uint8_t() = 0
    uint32_t read = (m_memory[address] | (m_memory[address + 1] << 8) | (m_memory[address + 2] << 16) |
                     (m_memory[address + 3] << 24));
    return read;
}

/**
 * @brief reset
 * Reset memory - erase all elements which is not part of the text segment of the program - suitable for resetting
 * the architecture (where data/stack is cleared, but the text segment persists)
 * @param textSize memory indices at positions LEQ textSize are not deleted
 */
void Memory::reset(uint32_t textSize) {
    for (auto it = m_memory.begin(); it != m_memory.end();) {
        // Use standard associative-container erasing. If erased, set iterator to iterator after the erased value
        // (default return value from .erase), else, increment iterator;
        if (it->first > textSize) {
            it = m_memory.erase(it);
        } else {
            ++it;
        }
    }
}
}  // namespace vsrtl
