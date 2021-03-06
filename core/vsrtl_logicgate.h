#ifndef VSRTL_LOGICGATE_H
#define VSRTL_LOGICGATE_H

#include "vsrtl_component.h"

namespace vsrtl {
namespace core {

template <unsigned int W, unsigned int nInputs>
class LogicGate : public Component {
public:
    LogicGate(std::string name, SimComponent* parent) : Component(name, parent) {}
    OUTPUTPORT(out, W);
    INPUTPORTS(in, W, nInputs);
};

template <unsigned int W, unsigned int nInputs>
class And : public LogicGate<W, nInputs> {
public:
    SetGraphicsType(And) And(std::string name, SimComponent* parent) : LogicGate<W, nInputs>(name, parent) {
        this->out << [=] {
            auto v = this->in[0]->template value<VSRTL_VT_U>();
            for (unsigned i = 1; i < this->in.size(); i++) {
                v = v & this->in[i]->template value<VSRTL_VT_U>();
            }
            return v;
        };
    }
};

template <unsigned int W, unsigned int nInputs>
class Nand : public LogicGate<W, nInputs> {
public:
    SetGraphicsType(Nand) Nand(std::string name, SimComponent* parent) : LogicGate<W, nInputs>(name, parent) {
        this->out << [=] {
            auto v = this->in[0]->template value<VSRTL_VT_U>();
            for (unsigned i = 1; i < this->in.size(); i++) {
                v = v & this->in[i]->template value<VSRTL_VT_U>();
            }
            return ~v;
        };
    }
};

template <unsigned int W, unsigned int nInputs>
class Or : public LogicGate<W, nInputs> {
public:
    SetGraphicsType(Or);
    Or(std::string name, SimComponent* parent) : LogicGate<W, nInputs>(name, parent) {
        this->out << [=] {
            auto v = this->in[0]->template value<VSRTL_VT_U>();
            for (unsigned i = 1; i < this->in.size(); i++) {
                v = v | this->in[i]->template value<VSRTL_VT_U>();
            }
            return v;
        };
    }
};

template <unsigned int W, unsigned int nInputs>
class Xor : public LogicGate<W, nInputs> {
public:
    SetGraphicsType(Xor);
    Xor(std::string name, SimComponent* parent) : LogicGate<W, nInputs>(name, parent) {
        this->out << [=] {
            auto v = this->in[0]->template value<VSRTL_VT_U>();
            for (unsigned i = 1; i < this->in.size(); i++) {
                v = v ^ this->in[i]->template value<VSRTL_VT_U>();
            }
            return v;
        };
    }
};

template <unsigned int W, unsigned int nInputs>
class Not : public LogicGate<W, nInputs> {
public:
    SetGraphicsType(Not);
    Not(std::string name, SimComponent* parent) : LogicGate<W, nInputs>(name, parent) {
        this->out << [=] { return ~this->in[0]->template value<VSRTL_VT_U>(); };
    }
};

}  // namespace core
}  // namespace vsrtl

#endif  // VSRTL_LOGICGATE_H
