#ifndef VSRTL_LOGICGATE_H
#define VSRTL_LOGICGATE_H

#include "vsrtl_component.h"

namespace vsrtl {

template <unsigned int W>
class LogicGate : public Component {
public:
    LogicGate(std::string name, Component* parent, unsigned int nInputs) : Component(name, parent) {
        in = this->createInputPorts<W>("in", nInputs);
    }
    OUTPUTPORT(out, W);
    INPUTPORTS(in, W);
};

DefineGraphicsProxy(And);
template <unsigned int W>
class And : public LogicGate<W> {
public:
    DefineTypeID(And) And(std::string name, Component* parent, unsigned int nInputs)
        : LogicGate<W>(name, parent, nInputs) {
        this->out << [=] {
            auto v = this->in[0]->template value<VSRTL_VT_U>();
            for (int i = 1; i < this->in.size(); i++) {
                v = v & this->in[i]->template value<VSRTL_VT_U>();
            }
            return v;
        };
    }
};

DefineGraphicsProxy(Or);
template <unsigned int W>
class Or : public LogicGate<W> {
public:
    DefineTypeID(Or);
    Or(std::string name, Component* parent, unsigned int nInputs) : LogicGate<W>(name, parent, nInputs) {
        this->out << [=] {
            auto v = this->in[0]->template value<VSRTL_VT_U>();
            for (int i = 1; i < this->in.size(); i++) {
                v = v | this->in[i]->template value<VSRTL_VT_U>();
            }
            return v;
        };
    }
};

DefineGraphicsProxy(Xor);
template <unsigned int W>
class Xor : public LogicGate<W> {
public:
    DefineTypeID(Xor);
    Xor(std::string name, Component* parent, unsigned int nInputs) : LogicGate<W>(name, parent, nInputs) {
        this->out << [=] {
            auto v = this->in[0]->template value<VSRTL_VT_U>();
            for (int i = 1; i < this->in.size(); i++) {
                v = v ^ this->in[i]->template value<VSRTL_VT_U>();
            }
            return v;
        };
    }
};

DefineGraphicsProxy(Not);
template <unsigned int W>
class Not : public LogicGate<W> {
public:
    DefineTypeID(Not);
    Not(std::string name, Component* parent) : LogicGate<W>(name, parent, 1) {
        this->out << [=] { return signextend(~this->in[0]->template value<VSRTL_VT_U>()); };
    }
};

}  // namespace vsrtl

#endif  // VSRTL_LOGICGATE_H
