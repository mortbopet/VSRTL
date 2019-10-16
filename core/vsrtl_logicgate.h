#ifndef VSRTL_LOGICGATE_H
#define VSRTL_LOGICGATE_H

#include "vsrtl_component.h"

namespace vsrtl {

template <unsigned int W, unsigned int nInputs>
class LogicGate : public Component {
public:
    LogicGate(std::string name, Component* parent) : Component(name, parent) {}
    OUTPUTPORT(out, W);
    INPUTPORTS(in, W, nInputs);
};

DefineGraphicsProxy(And);
template <unsigned int W, unsigned int nInputs>
class And : public LogicGate<W, nInputs> {
public:
    DefineTypeID(And) And(std::string name, Component* parent) : LogicGate<W, nInputs>(name, parent) {
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
template <unsigned int W, unsigned int nInputs>
class Or : public LogicGate<W, nInputs> {
public:
    DefineTypeID(Or);
    Or(std::string name, Component* parent) : LogicGate<W, nInputs>(name, parent) {
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
template <unsigned int W, unsigned int nInputs>
class Xor : public LogicGate<W, nInputs> {
public:
    DefineTypeID(Xor);
    Xor(std::string name, Component* parent) : LogicGate<W, nInputs>(name, parent) {
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
template <unsigned int W, unsigned int nInputs>
class Not : public LogicGate<W, nInputs> {
public:
    DefineTypeID(Not);
    Not(std::string name, Component* parent) : LogicGate<W, nInputs>(name, parent, 1) {
        this->out << [=] { return signextend(~this->in[0]->template value<VSRTL_VT_U>()); };
    }
};

}  // namespace vsrtl

#endif  // VSRTL_LOGICGATE_H
