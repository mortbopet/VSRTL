#include "vsrtl_component.h"

namespace vsrtl {

void Component::resetPropagation() {
    if (m_propagationState == PropagationState::unpropagated) {
        // Constants (components with no inputs) are always propagated
    } else {
        m_propagationState = PropagationState::unpropagated;
        for (const auto& i : m_inputports)
            i->resetPropagation();
        for (const auto& o : m_outputports)
            o->resetPropagation();
    }
}

void Component::addSubcomponent(Component* subcomponent) {
    m_subcomponents.push_back(std::unique_ptr<Component>(subcomponent));
}

void Component::propagateComponent(std::vector<Port*>& propagationStack) {
    // Component has already been propagated
    if (m_propagationState == PropagationState::propagated)
        return;

    if (isRegister()) {
        // Registers are implicitely clocked by calling propagate() on its output ports.
        /** @remark register <must> be saved before propagateComponent reaches the register ! */
        m_propagationState = PropagationState::propagated;
        for (const auto& s : m_outputports) {
            s->propagate(propagationStack);
        }
    } else {
        // All sequential logic must have their inputs propagated before they themselves can propagate. If this is
        // not the case, the function will return. Iff the circuit is correctly connected, this component will at a
        // later point be visited, given that the input port which is currently not yet propagated, will become
        // propagated at some point, signalling its connected components to propagate.
        for (const auto& input : m_inputports) {
            if (!input->isPropagated())
                return;
        }

        for (const auto& sc : m_subcomponents)
            sc->propagateComponent(propagationStack);

        // At this point, all input ports are assured to be propagated. In this case, it is safe to propagate
        // the outputs of the component.
        for (const auto& s : m_outputports) {
            s->propagate(propagationStack);
        }
        m_propagationState = PropagationState::propagated;

        // if any internal values have changed...
        // @todo: implement granular change signal emission
        changed.Emit();
    }

    // Signal all connected components of the current component to propagate
    for (const auto& out : m_outputports) {
        for (const auto& in : out->getOutputPorts()) {
            // With the input port of the connected component propagated, the parent component may be propagated.
            // This will succeed if all input components to the parent component has been propagated.
            in->getParent()->propagateComponent(propagationStack);

            // To facilitate output -> output connections, we need to trigger propagation in the output's parent
            // aswell
            /**
             * IN   IN   OUT  OUT
             *   _____________
             *  |    _____   |
             *  |   |    |   |
             *  |   |   ->--->
             *  |   |____|   |
             *  |____________|
             *
             */
            for (const auto& inout : in->getOutputPorts())
                inout->getParent()->propagateComponent(propagationStack);
        }
    }
}

void Component::verifyComponent() const {
    for (const auto& ip : m_inputports) {
        if (!ip->isConnected()) {
            throw std::runtime_error("A component has unconnected inputs");
        }
        if (ip->getWidth() == 0) {
            throw std::runtime_error(
                "A port did not have its width set. Parent component of port should set port width in its "
                "constructor");
        }
    }

    for (const auto& op : m_outputports) {
        if (op->getWidth() == 0) {
            throw std::runtime_error(
                "A port did not have its width set. Parent component of port should set port width in its "
                "constructor");
        }
    }
}

void Component::initialize() {
    if (m_inputports.size() == 0 && !hasSubcomponents()) {
        // Component has no input ports - ie. component is a constant. propagate all output ports and set component
        // as propagated.
        for (const auto& p : m_outputports)
            p->propagateConstant();
        m_propagationState = PropagationState::propagated;
    }
}

void Component::getComponentGraph(std::map<Component*, std::vector<Component*>>& componentGraph) {
    // Register adjacent components (child components) in the graph, and add subcomponents to graph
    componentGraph[this];
    for (const auto& c : m_subcomponents) {
        componentGraph[this].push_back(c.get());
        c->getComponentGraph(componentGraph);
    }
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
