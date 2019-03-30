#include "vsrtl_component.h"

namespace vsrtl {

void Component::addSubcomponent(Component* subcomponent) {
    m_subcomponents.push_back(std::unique_ptr<Component>(subcomponent));
}

std::vector<Port*> Component::createInputPorts(std::string name, unsigned int n, unsigned int width) {
    return createPorts(name, m_inputports, n, width);
}

std::vector<Port*> Component::createOutputPorts(std::string name, unsigned int n, unsigned int width) {
    return createPorts(name, m_outputports, n, width);
}

std::vector<Port*> Component::createPorts(std::string name, std::vector<std::unique_ptr<Port>>& container,
                                          unsigned int n, unsigned int width) {
    std::vector<Port*> ports;
    for (int i = 0; i < n; i++) {
        std::string i_name = name + "_" + std::to_string(i);
        Port* port = new Port(i_name.c_str(), this, width);
        container.push_back(std::unique_ptr<Port>(port));
        ports.push_back(port);
    }
    return ports;
}

Port& Component::createPort(std::string name, std::vector<std::unique_ptr<Port>>& container, unsigned int width) {
    Port* port = new Port(name, this, width);
    container.push_back(std::unique_ptr<Port>(port));
    return *port;
}

}  // namespace vsrtl
