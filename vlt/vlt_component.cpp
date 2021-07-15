#include "vlt_component.h"

#include "vlt_port.h"

#include <stdexcept>

namespace vsrtl {
namespace vlt {
Component::Component(const std::string& displayName, SimComponent* parent) : SimComponent(displayName, parent) {}

Port& Component::createInputPort(const VarRef& name, unsigned width) {
    return createPort(name, m_inputPorts, width, vsrtl::SimPort::PortType::in);
}
Port& Component::createOutputPort(const VarRef& name, unsigned width) {
    return createPort(name, m_outputPorts, width, vsrtl::SimPort::PortType::out);
}
Port& Component::createSignal(const VarRef& name, unsigned width) {
    return createPort(name, m_signals, width, vsrtl::SimPort::PortType::signal);
}

Port& Component::createPort(VarRef name, std::set<std::unique_ptr<SimPort>, PortBaseCompT>& container, unsigned width,
                            vsrtl::SimPort::PortType type) {
    verifyIsUniquePortName(name);
    Port* port = static_cast<Port*>((*container.emplace(std::make_unique<Port>(name, width, type, this)).first).get());

    return *port;
}

std::string Component::addSuffix(const std::string& type) {
    return type + "_" + std::to_string(m_componentSuffixes[type]++);
}

}  // namespace vlt
}  // namespace vsrtl
