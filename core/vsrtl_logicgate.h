#ifndef VSRTL_LOGICGATE_H
#define VSRTL_LOGICGATE_H

#include "vsrtl_component.h"

namespace vsrtl {

class LogicGate : public Component {
public:
    LogicGate(std::string name, unsigned int nInputs, unsigned int width, Component* parent)
        : Component(name, parent), m_width(width) {
        in = this->createInputPorts("in", nInputs);
        for (const auto& ip : in) {
            ip->setWidth(width);
        }
        out.setWidth(width);
    }
    OUTPUTPORT(out);
    INPUTPORTS(in);

protected:
    unsigned int m_width;
};

class And : public LogicGate {
public:
    std::type_index getTypeId() const override { return std::type_index(typeid(And)); }
    And(std::string name, unsigned int nInputs, unsigned int width, Component* parent)
        : LogicGate(name, nInputs, width, parent) {
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
    Or(std::string name, unsigned int nInputs, unsigned int width, Component* parent)
        : LogicGate(name, nInputs, width, parent) {
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
    Xor(std::string name, unsigned int nInputs, unsigned int width, Component* parent)
        : LogicGate(name, nInputs, width, parent) {
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
    Not(std::string name, unsigned int width, Component* parent) : LogicGate(name, 1, width, parent) {
        this->out << [=] { return signextend<VSRTL_VT_U>(~this->in[0]->template value<VSRTL_VT_U>(), width); };
    }
};

}  // namespace vsrtl

#endif  // VSRTL_LOGICGATE_H
