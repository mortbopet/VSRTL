#ifndef VSRTL_LOGICGATE_H
#define VSRTL_LOGICGATE_H

#include "vsrtl_component.h"

namespace vsrtl {

template <uint32_t width, uint32_t inputCount>
class LogicGate : public Component {
    NON_REGISTER_COMPONENT
    static_assert(inputCount > 0, "Input count must be greater than 0");

public:
    LogicGate(std::string name) : Component(name) {}
    OUTPUTPORT(out, width);
    INPUTPORTS(in, width, inputCount);
};

template <unsigned int inputCount, unsigned int width>
class And : public LogicGate<inputCount, width> {
public:
    And(std::string name = "&") : LogicGate<inputCount, width>(name) {
        this->out << [=] {
            auto v = this->in[0]->template value<VSRTL_VT_U>();
            for (int i = 1; i < this->in.size(); i++) {
                v = v & this->in[i]->template value<VSRTL_VT_U>();
            }
            return v;
        };
    }
};

template <unsigned int inputCount, unsigned int width>
class Or : public LogicGate<inputCount, width> {
public:
    Or(std::string name = "|") : LogicGate<inputCount, width>(name) {
        this->out << [=] {
            auto v = this->in[0]->template value<VSRTL_VT_U>();
            for (int i = 1; i < this->in.size(); i++) {
                v = v | this->in[i]->template value<VSRTL_VT_U>();
            }
            return v;
        };
    }
};

template <unsigned int inputCount, unsigned int width>
class Xor : public LogicGate<inputCount, width> {
public:
    const char* getBaseType() const override { return "Xor"; }
    Xor(std::string name = "^") : LogicGate<inputCount, width>(name) {
        this->out << [=] {
            auto v = this->in[0]->template value<VSRTL_VT_U>();
            for (int i = 1; i < this->in.size(); i++) {
                v = v ^ this->in[i]->template value<VSRTL_VT_U>();
            }
            return v;
        };
    }
};

template <unsigned int width>
class Not : public LogicGate<1, width> {
public:
    Not(std::string name = "&") : LogicGate<1, width>(name) {
        this->out << [=] { return ~this->in[0]->template value<VSRTL_VT_U>(); };
    }
};

}  // namespace vsrtl

#endif  // VSRTL_LOGICGATE_H
