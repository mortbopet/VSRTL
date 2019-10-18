#ifndef MEMORY_H
#define MEMORY_H

#include "vsrtl_component.h"
#include "vsrtl_defines.h"
#include "vsrtl_register.h"

#include <cstdint>
#include <unordered_map>

namespace vsrtl {

struct MemoryEviction {
    VSRTL_VT_U addr;
    VSRTL_VT_U data;
};

// DefineGraphicsProxy(Memory);
template <unsigned int addrWidth, unsigned int dataWidth>
class Memory : public ClockedComponent {
public:
    DefineTypeID(Component);
    Memory(std::string name, Component* parent) : ClockedComponent(name, parent) {
        data_out << [=] { return read(addr.template value<VSRTL_VT_U>()); };
    }

    void reset() override {
        m_memory.clear();
        m_rewindstack.clear();
    }

    void save() override {
        const VSRTL_VT_U addr_v = addr.template value<VSRTL_VT_U>();
        const VSRTL_VT_U data_in_v = data_in.template value<VSRTL_VT_U>();
        const VSRTL_VT_U data_out_v = read(addr_v);
        auto ev = MemoryEviction({addr_v, data_out_v});
        saveToStack(ev);
        write(addr_v, data_in_v);
    }

    void rewind() override {
        if (m_rewindstack.size() > 0) {
            auto lastEviction = m_rewindstack.front();
            write(lastEviction.addr, lastEviction.data);
            m_rewindstack.pop_front();
        }
    }

    INPUTPORT(addr, addrWidth);
    INPUTPORT(data_in, dataWidth);
    INPUTPORT(wr_en, 1);
    OUTPUTPORT(data_out, dataWidth);

private:
    std::unordered_map<VSRTL_VT_U, uint8_t> m_memory;
    std::deque<MemoryEviction> m_rewindstack;

    VSRTL_VT_U read(VSRTL_VT_U address) {
        // Note: If address is not found in memory map, a default constructed object
        // will be created, and read. in our case uint8_t() = 0
        VSRTL_VT_U read = (m_memory[address] | (m_memory[address + 1] << 8) | (m_memory[address + 2] << 16) |
                           (m_memory[address + 3] << 24));
        return read;
    }

    void write(VSRTL_VT_U address, VSRTL_VT_U value, int size = sizeof(VSRTL_VT_U)) {
        // writes value from the given address start, and up to $size bytes of
        // $value
        for (int i = 0; i < size; i++) {
            m_memory[address + i] = value & 0xff;
            value >>= 8;
        }
    }

    void saveToStack(MemoryEviction v) {
        m_rewindstack.push_front(v);
        if (m_rewindstack.size() > rewindStackSize()) {
            m_rewindstack.pop_back();
        }
    }
};
}  // namespace vsrtl

#endif  // MEMORY_H
