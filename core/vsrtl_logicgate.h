#ifndef VSRTL_LOGICGATE_H
#define VSRTL_LOGICGATE_H

#include "vsrtl_component.h"

namespace vsrtl {

class LogicGate : public Component {
public:
    LogicGate(std::string name, unsigned int nInputs, unsigned int width) : Component(name), m_width(width) {
        in = this->createInputPorts("in", nInputs);
        for (const auto& ip : in) {
            ip->setWidth(width);
        }
        out.setWidth(1);
    }
    OUTPUTPORT(out);
    INPUTPORTS(in);

protected:
    unsigned int m_width;
};

class And : public LogicGate {
public:
    std::type_index getTypeId() const override { return std::type_index(typeid(And)); }
    And(std::string name, unsigned int nInputs, unsigned int width) : LogicGate(name, nInputs, width) {
        this->out << [=] {
            auto v = this->in[0]->template value<VSRTL_VT_U>();
            for (int i = 1; i < this->in.size(); i++) {
                v = v & this->in[i]->template value<VSRTL_VT_U>();
            }
            return v;
        };
    }
};

class Or : public LogicGate {
public:
    std::type_index getTypeId() const override { return std::type_index(typeid(Or)); }
    Or(std::string name, unsigned int nInputs, unsigned int width) : LogicGate(name, nInputs, width) {
        this->out << [=] {
            auto v = this->in[0]->template value<VSRTL_VT_U>();
            for (int i = 1; i < this->in.size(); i++) {
                v = v | this->in[i]->template value<VSRTL_VT_U>();
            }
            return v;
        };
    }
};

class Xor : public LogicGate {
public:
    std::type_index getTypeId() const override { return std::type_index(typeid(Xor)); }
    Xor(std::string name, unsigned int nInputs, unsigned int width) : LogicGate(name, nInputs, width) {
        this->out << [=] {
            auto v = this->in[0]->template value<VSRTL_VT_U>();
            for (int i = 1; i < this->in.size(); i++) {
                v = v ^ this->in[i]->template value<VSRTL_VT_U>();
            }
            return v;
        };
    }
};

class Not : public LogicGate {
public:
    std::type_index getTypeId() const override { return std::type_index(typeid(Not)); }
    Not(std::string name, unsigned int width) : LogicGate(name, 1, width) {
        this->out << [=] { return ~this->in[0]->template value<VSRTL_VT_U>(); };
    }
};

}  // namespace vsrtl

#endif  // VSRTL_LOGICGATE_H
