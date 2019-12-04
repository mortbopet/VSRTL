#ifndef MEMORY_H
#define MEMORY_H

#include "vsrtl_component.h"
#include "vsrtl_defines.h"
#include "vsrtl_register.h"

#include <cstdint>
#include <unordered_map>

namespace vsrtl {
namespace core {

struct MemoryEviction {
    VSRTL_VT_U addr;
    VSRTL_VT_U data;
};

using SparseArrayMemory = std::unordered_map<VSRTL_VT_U, uint8_t>;

template <unsigned int addrWidth, unsigned int dataWidth>
class WrMemory : public ClockedComponent {
    template <unsigned int, unsigned int>
    friend class Memory;

public:
    SetGraphicsType(Component);
    WrMemory(std::string name, Component* parent) : ClockedComponent(name, parent) {}
    void reset() override {
        m_memory->clear();
        m_rewindstack.clear();

        // Rewrite all initializations to memory
        for (const auto& m : m_initMemories) {
            m_memory->insert(m.begin(), m.end());
        }
    }

    void save() override {
        const VSRTL_VT_U addr_v = addr.template value<VSRTL_VT_U>();
        const VSRTL_VT_U data_in_v = data_in.template value<VSRTL_VT_U>();
        const VSRTL_VT_U data_out_v = read(*m_memory, addr_v);
        auto ev = MemoryEviction({addr_v, data_out_v});
        saveToStack(ev);
        if (static_cast<bool>(wr_en))
            write(*m_memory, addr_v, data_in_v);
    }

    void rewind() override {
        if (m_rewindstack.size() > 0) {
            auto lastEviction = m_rewindstack.front();
            write(*m_memory, lastEviction.addr, lastEviction.data);
            m_rewindstack.pop_front();
        }
    }

    INPUTPORT(addr, addrWidth);
    INPUTPORT(data_in, dataWidth);
    INPUTPORT(wr_en, 1);

private:
    void setMemory(SparseArrayMemory* mem) { m_memory = mem; }

    VSRTL_VT_U read(std::unordered_map<VSRTL_VT_U, uint8_t>& memory, VSRTL_VT_U address) {
        // Note: If address is not found in memory map, a default constructed object
        // will be created, and read. in our case uint8_t() = 0
        VSRTL_VT_U read =
            (memory[address] | (memory[address + 1] << 8) | (memory[address + 2] << 16) | (memory[address + 3] << 24));
        return read;
    }

    void write(std::unordered_map<VSRTL_VT_U, uint8_t>& memory, VSRTL_VT_U address, VSRTL_VT_U value,
               int size = sizeof(VSRTL_VT_U)) {
        // writes value from the given address start, and up to $size bytes of
        // $value
        for (int i = 0; i < size; i++) {
            memory[address + i] = value & 0xff;
            value >>= 8;
        }
    }

    void saveToStack(MemoryEviction v) {
        m_rewindstack.push_front(v);
        if (m_rewindstack.size() > rewindStackSize()) {
            m_rewindstack.pop_back();
        }
    }

    std::vector<SparseArrayMemory> m_initMemories;
    SparseArrayMemory* m_memory = nullptr;
    std::deque<MemoryEviction> m_rewindstack;
};

template <unsigned int addrWidth, unsigned int dataWidth>
class RdMemory : public Component {
    template <unsigned int, unsigned int>
    friend class Memory;

public:
    SetGraphicsType(ClockedComponent);
    RdMemory(std::string name, Component* parent) : Component(name, parent) {
        data_out << [=] { return read(addr.template value<VSRTL_VT_U>()); };
    }

    INPUTPORT(addr, addrWidth);
    OUTPUTPORT(data_out, dataWidth);

private:
    void setMemory(SparseArrayMemory* mem) { m_memory = mem; }
    VSRTL_VT_U read(VSRTL_VT_U address) {
        // Note: If address is not found in memory map, a default constructed object
        // will be created, and read. in our case uint8_t() = 0
        VSRTL_VT_U read = ((*m_memory)[address] | ((*m_memory)[address + 1] << 8) | ((*m_memory)[address + 2] << 16) |
                           ((*m_memory)[address + 3] << 24));
        return read;
    }

    SparseArrayMemory* m_memory = nullptr;
};

template <unsigned int addrWidth, unsigned int dataWidth>
class Memory : public Component {
public:
    SetGraphicsType(ClockedComponent);
    Memory(std::string name, Component* parent) : Component(name, parent) {
        _wr_mem->setMemory(&m_memory);
        _rd_mem->setMemory(&m_memory);
        addr >> _wr_mem->addr;
        wr_en >> _wr_mem->wr_en;
        data_in >> _wr_mem->data_in;

        addr >> _rd_mem->addr;
        _rd_mem->data_out >> data_out;
    }

    /**
     * @brief addInitializationMemory
     * The specified program will be added as a memory segment which will be loaded into this memory once it is reset.
     */
    template <typename T>
    void addInitializationMemory(const VSRTL_VT_U startAddr, T* program, size_t n) {
        auto& mem = _wr_mem->m_initMemories.emplace_back();
        VSRTL_VT_U addr = startAddr;
        for (size_t i = 0; i < n; i++) {
            _wr_mem->write(mem, addr, program[i], sizeof(T));
            addr += sizeof(T);
        }
    }

    SUBCOMPONENT(_wr_mem, TYPE(WrMemory<addrWidth, dataWidth>));
    SUBCOMPONENT(_rd_mem, TYPE(RdMemory<addrWidth, dataWidth>));

    INPUTPORT(addr, addrWidth);
    INPUTPORT(data_in, dataWidth);
    INPUTPORT(wr_en, 1);
    OUTPUTPORT(data_out, dataWidth);

private:
    std::unordered_map<VSRTL_VT_U, uint8_t> m_memory;
};

template <unsigned int addrWidth, unsigned int dataWidth>
class ROM : public Component {
public:
    SetGraphicsType(ClockedComponent);
    ROM(std::string name, Component* parent) : Component(name, parent) {
        data_out << [=] { return read(m_memory, addr.template value<VSRTL_VT_U>()); };
    }

    /**
     * @brief addInitializationMemory
     * The specified program will be added as a memory segment which will be loaded into this memory once it is reset.
     */
    template <typename T>
    void initializeMemory(const VSRTL_VT_U startAddr, T* program, size_t n) {
        VSRTL_VT_U addr = startAddr;
        for (size_t i = 0; i < n; i++) {
            write(m_memory, addr, program[i], sizeof(T));
            addr += sizeof(T);
        }
    }

    INPUTPORT(addr, addrWidth);
    OUTPUTPORT(data_out, dataWidth);

private:
    std::vector<std::unordered_map<VSRTL_VT_U, uint8_t>> m_initMemories;
    std::unordered_map<VSRTL_VT_U, uint8_t> m_memory;

    VSRTL_VT_U read(std::unordered_map<VSRTL_VT_U, uint8_t>& memory, VSRTL_VT_U address) {
        // Note: If address is not found in memory map, a default constructed object
        // will be created, and read. in our case uint8_t() = 0
        VSRTL_VT_U read =
            (memory[address] | (memory[address + 1] << 8) | (memory[address + 2] << 16) | (memory[address + 3] << 24));
        return read;
    }

    void write(std::unordered_map<VSRTL_VT_U, uint8_t>& memory, VSRTL_VT_U address, VSRTL_VT_U value,
               int size = sizeof(VSRTL_VT_U)) {
        // writes value from the given address start, and up to $size bytes of
        // $value
        for (int i = 0; i < size; i++) {
            memory[address + i] = value & 0xff;
            value >>= 8;
        }
    }
};

}  // namespace core
}  // namespace vsrtl

#endif  // MEMORY_H
