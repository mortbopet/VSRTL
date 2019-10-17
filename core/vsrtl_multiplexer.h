#ifndef VSRTL_MULTIPLEXER_H
#define VSRTL_MULTIPLEXER_H

#include <array>
#include "vsrtl_component.h"
#include "vsrtl_defines.h"
#include "vsrtl_enum.h"

namespace vsrtl {

class MultiplexerBase : public Component {
public:
    MultiplexerBase(std::string name, Component* parent) : Component(name, parent) {}

    virtual std::vector<PortBase*> getIns() = 0;
    virtual PortBase* getSelect() = 0;
    virtual PortBase* getOut() = 0;
};

DefineGraphicsProxy(Multiplexer);
template <unsigned int N, unsigned int W>
class Multiplexer : public MultiplexerBase {
public:
    DefineTypeID(Multiplexer);
    Multiplexer(std::string name, Component* parent) : MultiplexerBase(name, parent) {
        out << [=] { return ins.at(select.template value<VSRTL_VT_U>())->template value<VSRTL_VT_U>(); };
    }

    std::vector<PortBase*> getIns() override {
        std::vector<PortBase*> ins_base;
        for (const auto& in : ins)
            ins_base.push_back(in);
        return ins_base;
    }

    /**
     * @brief others
     * @return a vector of all ports which has not been connected
     */
    std::vector<Port<W>*> others() {
        std::vector<Port<W>*> vec;
        for (const auto& port : ins) {
            if (!port->getInputPort()) {
                vec.push_back(port);
            }
        }
        return vec;
    }

    PortBase* getSelect() override { return &select; }
    PortBase* getOut() override { return &out; }

    OUTPUTPORT(out, W);
    INPUTPORT(select, ceillog2(N));
    INPUTPORTS(ins, W, N);
};

template <typename E_t, unsigned int W>
class EnumMultiplexer : public Multiplexer<E_t::count(), W> {
public:
    EnumMultiplexer(std::string name, Component* parent) : Multiplexer<E_t::count(), W>(name, parent) {
        for (unsigned int v = E_t::get().begin(); v != E_t::get().count(); v++) {
            m_enumToPort[v] = this->ins.at(v);
        }
    }

    Port<W>& get(int enumIdx) {
        if (m_enumToPort.count(enumIdx) != 1) {
            throw std::runtime_error("Requested index out of Enum range");
        }
        if (m_enumToPort[enumIdx] == nullptr) {
            throw std::runtime_error("Requested enum index not associated with any port");
        }
        return *m_enumToPort[enumIdx];
    }

private:
    std::map<int, Port<W>*> m_enumToPort;
};

}  // namespace vsrtl

#endif  // VSRTL_MULTIPLEXER_H
