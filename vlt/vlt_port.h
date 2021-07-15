#pragma once

#include "../interface/vsrtl_interface.h"

namespace vsrtl {
namespace vlt {

class Port : public SimPort {
public:
    Port(const std::string& name, unsigned width, PortType type, SimComponent* parent);

    unsigned int getWidth() const override { return m_width; }
    VSRTL_VT_U uValue() const override { return isConstant() ? m_constantValue : 0; }
    VSRTL_VT_S sValue() const override { return 0; }

    bool isConstant() const override { return m_isConstant; }
    void setConstant(bool _isConstant, VSRTL_VT_U value) {
        m_isConstant = _isConstant;
        m_constantValue = value;
    }

    void operator>>(Port& toThis) {
        m_outputPorts.push_back(&toThis);
        if (toThis.m_inputPort != nullptr) {
            throw std::runtime_error(
                "Failed trying to connect port '" + getParent()->getName() + ":" + getName() + "' to port '" +
                toThis.getParent()->getName() + ":" + toThis.getName() + ". Port is already connected to '" +
                toThis.getInputPort()->getParent()->getName() + ":" + toThis.getInputPort()->getName());
        }
        toThis.m_inputPort = this;
    }

private:
    unsigned m_width;
    bool m_isConstant = false;

    // Mutually exclusive objects for value representation
    union {
        VSRTL_VT_U m_constantValue;
    };
};

}  // namespace vlt
}  // namespace vsrtl
