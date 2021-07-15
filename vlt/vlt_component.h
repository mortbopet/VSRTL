#pragma once

#include "../interface/vsrtl_interface.h"

namespace vsrtl {
namespace vlt {

class Port;

class Component : public SimComponent {
public:
    using VarRef = std::string;
    SetGraphicsType(Component);
    Component(const std::string& displayName, SimComponent* parent);

    Port& createInputPort(const VarRef& name, unsigned width);
    Port& createOutputPort(const VarRef& name, unsigned width);
    Port& createSignal(const VarRef& name, unsigned width);

    /**
     * @brief getSuffix
     * Returns a unique suffix which may be appended to subcomponent @p type, such that its name will be unique within
     * this component.
     */
    std::string addSuffix(const std::string& type);

private:
    Port& createPort(std::string name, std::set<std::unique_ptr<SimPort>, PortBaseCompT>& container, unsigned width,
                     vsrtl::SimPort::PortType);

    std::map<std::string, unsigned> m_componentSuffixes;
    /**
     * @brief m_vars
     * Mapping from Verilator <var> tags to the associated port.
     */
    std::map<VarRef, Port*> m_vars;
};
}  // namespace vlt
}  // namespace vsrtl
