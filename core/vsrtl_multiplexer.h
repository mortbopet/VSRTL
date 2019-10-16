#ifndef VSRTL_MULTIPLEXER_H
#define VSRTL_MULTIPLEXER_H

#include <array>
#include "vsrtl_component.h"
#include "vsrtl_defines.h"

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
        ins = createInputPorts<W>("in", N);

        out << [=] { return ins[select.template value<VSRTL_VT_U>()]->template value<VSRTL_VT_U>(); };
    }

    std::vector<PortBase*> getIns() override {
        std::vector<PortBase*> ins_base;
        for (const auto& in : ins)
            ins_base.push_back(in);
        return ins_base;
    }

    PortBase* getSelect() override { return &select; }
    PortBase* getOut() override { return &out; }

    OUTPUTPORT(out, W);
    INPUTPORT(select, ceillog2(N));
    INPUTPORTS(ins, W);
};
}  // namespace vsrtl

#endif  // VSRTL_MULTIPLEXER_H
