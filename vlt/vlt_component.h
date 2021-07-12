#pragma once

#include "../interface/vsrtl_interface.h"

namespace vsrtl {
namespace vlt {

class Port;

class Component : public SimComponent {
public:
    SetGraphicsType(Component);
    Component(const std::string& displayName, SimComponent* parent);

    Port& createInputPort(const std::string& name, unsigned width);
    Port& createOutputPort(const std::string& name, unsigned width);

    /**
     * @brief getSuffix
     * Returns a unique suffix which may be appended to subcomponent @p type, such that its name will be unique within
     * this component.
     */
    std::string addSuffix(const std::string& type);
    Port* portForVar(const std::string& var);
    void addVar(const std::string& var, Port*);

private:
    Port& createPort(std::string name, std::set<std::unique_ptr<SimPort>, PortBaseCompT>& container, unsigned width);

    std::map<std::string, unsigned> m_componentSuffixes;
    /**
     * @brief m_vars
     * Mapping from Verilator <var> tags to the associated port.
     */
    std::map<std::string, Port*> m_vars;
};
}  // namespace vlt
}  // namespace vsrtl
