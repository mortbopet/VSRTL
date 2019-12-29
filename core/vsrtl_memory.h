#ifndef MEMORY_H
#define MEMORY_H

#include "vsrtl_component.h"
#include "vsrtl_defines.h"
#include "vsrtl_register.h"

#include "../interface/vsrtl_gfxobjecttypes.h"

#include <cstdint>
#include <unordered_map>

namespace vsrtl {
namespace core {

struct MemoryEviction {
    VSRTL_VT_U addr;
    VSRTL_VT_U data;
    VSRTL_VT_U width;
};

using SparseArrayMemory = std::unordered_map<VSRTL_VT_U, uint8_t>;

template <unsigned addrWidth, unsigned dataWidth, bool byteIndexed = true>
class BaseMemory {
public:
    BaseMemory() {}

    void setMemory(SparseArrayMemory* mem) { m_memory = mem; }

    VSRTL_VT_U read(VSRTL_VT_U address) {
        // Note: If address is not found in memory map, a default constructed object
        // will be created, and read. in our case uint8_t() = 0
        if constexpr (byteIndexed) {
            return ((*m_memory)[address] | ((*m_memory)[address + 1] << 8) | ((*m_memory)[address + 2] << 16) |
                    ((*m_memory)[address + 3] << 24));
        } else {
            return ((*m_memory)[(address << 2)] | ((*m_memory)[(address << 2) + 1] << 8) |
                    ((*m_memory)[(address << 2) + 2] << 16) | ((*m_memory)[(address << 2) + 3] << 24));
        }
    }

    void write(VSRTL_VT_U address, VSRTL_VT_U value, int size = sizeof(VSRTL_VT_U)) {
        if constexpr (byteIndexed) {
            writeMem(m_memory, address, value, size);
        } else {
            writeMem(m_memory, address << 2, value, size);
        }
    }

    /**
     * @brief addInitializationMemory
     * The specified program will be added as a memory segment which will be loaded into this memory once it is reset.
     */
    template <typename T>
    void addInitializationMemory(const VSRTL_VT_U startAddr, T* program, size_t n) {
        auto& mem = m_initMemories.emplace_back();
        VSRTL_VT_U addr = startAddr;
        for (size_t i = 0; i < n; i++) {
            // Add to initialization memories for future rewriting upon reset
            this->writeMem(&mem, addr, program[i], sizeof(T));
            // Write to active memory
            write(addr, program[i], sizeof(T));
            addr += sizeof(T);
        }
    }

protected:
    SparseArrayMemory* m_memory = nullptr;
    std::vector<SparseArrayMemory> m_initMemories;
    void writeMem(SparseArrayMemory* mem, VSRTL_VT_U address, VSRTL_VT_U value, int size = sizeof(VSRTL_VT_U)) {
        // writes value from the given address start, and up to $size bytes of
        // $value
        for (int i = 0; i < size; i++) {
            (*mem)[address + i] = value & 0xff;
            value >>= 8;
        }
    }
};

template <unsigned int addrWidth, unsigned int dataWidth, bool byteIndexed = true>
class WrMemory : public ClockedComponent, public BaseMemory<addrWidth, dataWidth, byteIndexed> {
public:
    SetGraphicsType(Component);
    WrMemory(std::string name, SimComponent* parent) : ClockedComponent(name, parent) {}
    void reset() override {
        this->m_memory->clear();
        m_rewindstack.clear();

        // Rewrite all initializations to memory
        for (const auto& m : this->m_initMemories) {
            this->m_memory->insert(m.begin(), m.end());
        }
    }

    void save() override {
        const VSRTL_VT_U addr_v = addr.template value<VSRTL_VT_U>();
        const VSRTL_VT_U data_in_v = data_in.template value<VSRTL_VT_U>();
        const VSRTL_VT_U data_out_v = this->read(addr_v);
        auto ev = MemoryEviction({addr_v, data_out_v, wr_width.uValue()});
        saveToStack(ev);
        if (static_cast<bool>(wr_en))
            this->write(addr_v, data_in_v, wr_width.uValue());
    }

    void rewind() override {
        if (m_rewindstack.size() > 0) {
            auto lastEviction = m_rewindstack.front();
            this->write(lastEviction.addr, lastEviction.data, lastEviction.width);
            m_rewindstack.pop_front();
        }
    }

    void forceValue(VSRTL_VT_U addr, VSRTL_VT_U value) override {
        assert(false && "todo: Forcing values on the rewind stack?");
        this->write(addr, value);
    }

    INPUTPORT(addr, addrWidth);
    INPUTPORT(data_in, dataWidth);
    INPUTPORT(wr_width, ceillog2(dataWidth / 8 + 1));  // # bytes
    INPUTPORT(wr_en, 1);

    void saveToStack(MemoryEviction v) {
        m_rewindstack.push_front(v);
        if (m_rewindstack.size() > rewindStackSize()) {
            m_rewindstack.pop_back();
        }
    }

    std::deque<MemoryEviction> m_rewindstack;
};

template <unsigned int addrWidth, unsigned int dataWidth, bool byteIndexed = true>
class RdMemory : public Component, public BaseMemory<addrWidth, dataWidth, byteIndexed> {
    template <unsigned int, unsigned int>
    friend class Memory;

public:
    SetGraphicsType(ClockedComponent);
    RdMemory(std::string name, SimComponent* parent) : Component(name, parent) {
        data_out << [=] { return this->read(addr.template value<VSRTL_VT_U>()); };
    }

    INPUTPORT(addr, addrWidth);
    OUTPUTPORT(data_out, dataWidth);
};

template <unsigned int addrWidth, unsigned int dataWidth>
class Memory : public Component {
public:
    SetGraphicsType(ClockedComponent);
    Memory(std::string name, SimComponent* parent) : Component(name, parent) {
        _wr_mem->setMemory(&m_memory);
        _rd_mem->setMemory(&m_memory);
        addr >> _wr_mem->addr;
        wr_en >> _wr_mem->wr_en;
        data_in >> _wr_mem->data_in;
        wr_width >> _wr_mem->wr_width;

        addr >> _rd_mem->addr;
        _rd_mem->data_out >> data_out;
    }

    SUBCOMPONENT(_wr_mem, TYPE(WrMemory<addrWidth, dataWidth>));
    SUBCOMPONENT(_rd_mem, TYPE(RdMemory<addrWidth, dataWidth>));

    INPUTPORT(addr, addrWidth);
    INPUTPORT(data_in, dataWidth);
    INPUTPORT(wr_en, 1);
    INPUTPORT(wr_width, ceillog2(dataWidth / 8 + 1));  // # bytes
    OUTPUTPORT(data_out, dataWidth);

private:
    std::unordered_map<VSRTL_VT_U, uint8_t> m_memory;
};

template <unsigned int addrWidth, unsigned int dataWidth, bool byteIndexed = true>
class ROM : public RdMemory<addrWidth, dataWidth, byteIndexed> {
public:
    ROM(std::string name, SimComponent* parent) : RdMemory<addrWidth, dataWidth, byteIndexed>(name, parent) {
        this->setMemory(&m_memory);
    }

private:
    std::unordered_map<VSRTL_VT_U, uint8_t> m_memory;
};

}  // namespace core
}  // namespace vsrtl

#endif  // MEMORY_H
