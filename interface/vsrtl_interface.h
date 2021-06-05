#pragma once

#include <assert.h>
#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <type_traits>
#include <typeindex>
#include <vector>

#include "Signals/Signal.h"
#include "vsrtl_defines.h"
#include "vsrtl_gfxobjecttypes.h"
#include "vsrtl_parameter.h"

namespace vsrtl {

// Forward declare everything
class SimBase;
class SimPort;
class SimComponent;
class SimDesign;
class SimSynchronous;

class SimBase {
public:
    SimBase(std::string name, SimBase* parent) : m_name(name), m_parent(parent) {}
    virtual ~SimBase() {}

    SimDesign* getDesign();

    template <typename T = std::runtime_error>
    void throwError(const std::string message) const {
        throw T(getName() + ": " + message);
    }

    const std::string& getName() const { return m_name; }
    const std::string& getDisplayName() const { return m_displayName.empty() ? m_name : m_displayName; }

    template <typename T = SimBase>
    T* getParent() const {
        return dynamic_cast<T*>(m_parent);
    }

    void setDisplayName(std::string name) { m_displayName = name; }

    template <typename T>
    void registerGraphic(T* obj) {
        if (m_graphicObject != nullptr) {
            throw std::runtime_error("Graphic object already registered");
        }
        m_graphicObject = obj;
    }

    template <typename T>
    T* getGraphic() const {
        return static_cast<T*>(m_graphicObject);
    }

protected:
    std::string m_name;
    SimBase* m_parent = nullptr;
    SimDesign* m_design = nullptr;
    std::string m_displayName;
    void* m_graphicObject = nullptr;
};

template <typename T>
struct BaseSorter {
    bool operator()(const T& lhs, const T& rhs) const {
        return std::lexicographical_compare(lhs->getName().begin(), lhs->getName().end(), rhs->getName().begin(),
                                            rhs->getName().end());
    }
};

class SimPort : public SimBase {
public:
    enum class Direction { in, out };
    SimPort(std::string name, SimBase* parent) : SimBase(name, parent) {}
    virtual ~SimPort() {}
    virtual unsigned int getWidth() const = 0;
    virtual VSRTL_VT_U uValue() const = 0;
    virtual VSRTL_VT_S sValue() const = 0;

    template <typename T = SimPort>
    std::vector<T*> getOutputPorts() {
        static_assert(std::is_base_of<SimPort, T>::value, "Must cast to a simulator-specific port type");
        if constexpr (std::is_same<T, SimPort>::value) {
            return m_outputPorts;
        } else {
            std::vector<T*> outputPorts;
            for (const auto& p : m_outputPorts) {
                outputPorts.push_back(p->cast<T>());
            }
            return outputPorts;
        }
    }

    template <typename T = SimPort>
    T* getInputPort() {
        static_assert(std::is_base_of<SimPort, T>::value, "Must cast to a simulator-specific port type");
        return m_inputPort->cast<T>();
    }

    virtual bool isConstant() const = 0;

    template <typename T>
    T* cast() {
        static_assert(std::is_base_of<SimPort, T>::value, "Must cast to a simulator-specific port type");
        if constexpr (std::is_same<T, SimPort>::value) {
            return this;
        } else {
            return static_cast<T*>(this);
        }
    }

    /* traverse from any given port towards its root (source) port, while executing nodeFunc(args...) in each port
    which is visited along the way*/
    template <typename T = SimPort, typename F, typename... Args>
    void traverseToRoot(const F& nodeFunc, Args&... args) {
        static_assert(std::is_base_of<SimPort, T>::value, "Must cast to a simulator-specific port type");
        nodeFunc(this, args...);
        if (getInputPort()) {
            getInputPort<T>()->traverseToRoot(nodeFunc, args...);
        }
    }

    /* From this port, visit all directly and implicitely connected port to this port, and execute nodeFunc(args...)
    in each visited port */
    template <typename T = SimPort, typename F, typename... Args>
    void traverseConnection(const F& nodeFunc, Args&... args) {
        static_assert(std::is_base_of<SimPort, T>::value, "Must cast to a simulator-specific port type");
        if (m_traversingConnection)
            return;
        m_traversingConnection = true;

        nodeFunc(this, args...);
        if (getInputPort()) {
            getInputPort<T>()->traverseConnection(nodeFunc, args...);
        }
        for (const auto& p : getOutputPorts<T>()) {
            p->traverseConnection(nodeFunc, args...);
        }

        m_traversingConnection = false;
    }

    /* Traverse from any given port towards its endpoint sinks, executing nodeFunc(args...) in each visited port */
    template <typename T = SimPort, typename F, typename... Args>
    void traverseToSinks(const F& nodeFunc, Args&... args) {
        static_assert(std::is_base_of<SimPort, T>::value, "Must cast to a simulator-specific port type");
        nodeFunc(this, args...);
        for (const auto& p : getOutputPorts<T>()) {
            p->traverseToSinks(nodeFunc, args...);
        }
    }

    template <typename T = SimPort>
    std::vector<T*> getPortsInConnection() {
        static_assert(std::is_base_of<SimPort, T>::value, "Must cast to a simulator-specific port type");
        std::vector<T*> portsInConnection;
        traverseConnection([=](T* port, std::vector<T*>& ports) { ports.push_back(port); }, portsInConnection);
        return portsInConnection;
    }

    /** @todo: Figure out whether these should be defined in the interface */
    /**
     * @brief stringValue
     * A port may define special string formatting to be displayed in the graphical library. If so, owning
     * components should set the string value function to provide such values.
     */
    virtual bool isEnumPort() const { return false; }
    virtual std::string valueToEnumString() const { throw std::runtime_error("This is not an enum port!"); }
    virtual VSRTL_VT_U enumStringToValue(const char*) const { throw std::runtime_error("This is not an enum port!"); }

    Gallant::Signal0<> changed;

protected:
    std::vector<SimPort*> m_outputPorts;
    SimPort* m_inputPort = nullptr;

private:
    bool m_traversingConnection = false;
};

#define TYPE(...) __VA_ARGS__
#define SUBCOMPONENT(name, type, ...) type* name = create_component<type>(#name, ##__VA_ARGS__)
#define SUBCOMPONENTS(name, type, n, ...) std::vector<type*> name = create_components<type>(#name, n, ##__VA_ARGS__)
#define PARAMETER(name, type, initial) Parameter<type>& name = this->template createParameter<type>(#name, initial)

namespace {
struct NoPredicate {};
}  // namespace

class SimComponent : public SimBase {
public:
    using PortBaseCompT = BaseSorter<std::unique_ptr<SimPort>>;
    using ComponentCompT = BaseSorter<std::unique_ptr<SimComponent>>;

    SimComponent(std::string name, SimBase* parent) : SimBase(name, parent) {}
    virtual ~SimComponent() {}

    /**
     * @brief getBaseType
     * Used to identify the component type, which is used when determining how to draw a component. Introduced to
     * avoid intermediate base classes for many (All) components, which is a template class. For instance, it is
     * desireable to identify all instances of "Constant<...>" objects, but without introducing a "BaseConstant"
     * class.
     * @return String identifier for the component type
     */
    virtual std::type_index getGraphicsID() const { return GraphicsIDFor(Component); }
    virtual const GraphicsType* getGraphicsType() const { return GraphycsTypeForComponent(Component)::get(); }

    /**
     * getInput&OutputComponents does not return a set, although it naturally should. In partitioning the circuit
     * graph, it is beneficial to know whether two components have multiple edges between each other.
     */
    template <typename T = SimComponent>
    std::vector<T*> getInputComponents() const {
        static_assert(std::is_base_of<SimComponent, T>::value, "Must cast to a simulator-specific component type");
        std::vector<T*> v;
        for (const auto& s : m_inputPorts) {
            v.push_back(s->getInputPort()->getParent<T>());
        }
        return v;
    }

    template <typename T = SimComponent>
    std::vector<T*> getOutputComponents() const {
        static_assert(std::is_base_of<SimComponent, T>::value, "Must cast to a simulator-specific component type");
        std::vector<T*> v;
        for (const auto& p : m_outputPorts) {
            for (const auto& pc : p->getOutputPorts())
                v.push_back(pc->getParent<T>());
        }
        return v;
    }

    template <SimPort::Direction d, typename T = SimPort>
    std::vector<T*> getPorts() const {
        static_assert(std::is_base_of<SimPort, T>::value, "Must cast to a simulator-specific port type");
        std::vector<T*> ports;
        if constexpr (d == SimPort::Direction::in) {
            for (const auto& p : m_inputPorts)
                ports.push_back(p->cast<T>());
        } else {
            for (const auto& p : m_outputPorts)
                ports.push_back(p->cast<T>());
        }
        return ports;
    }

    template <typename T = SimPort>
    std::vector<T*> getAllPorts() const {
        static_assert(std::is_base_of<SimPort, T>::value, "Must cast to a simulator-specific port type");
        std::vector<T*> ports;
        for (auto portsForDir : {getPorts<SimPort::Direction::in, T>(), getPorts<SimPort::Direction::out, T>()}) {
            ports.insert(ports.end(), portsForDir.begin(), portsForDir.end());
        }
        return ports;
    }

    void verifyHasSpecialPortID(const std::string& id) const {
        const auto* type = getGraphicsType();
        if (!type->hasSpecialPortID(id)) {
            throwError("Special port ID '" + id + "' is not a special port of the graphics type '" + type->getName() +
                       "'");
        }
    }

    template <typename T = SimPort>
    T* getSpecialPort(const std::string& id) const {
        static_assert(std::is_base_of<SimPort, T>::value, "Must cast to a simulator-specific port type");
        verifyHasSpecialPortID(id);
        if (m_specialPorts.count(id) != 0)
            return m_specialPorts.at(id);
        return nullptr;
    }

    template <typename T = SimPort>
    std::vector<T*> getSpecialPorts() const {
        static_assert(std::is_base_of<SimPort, T>::value, "Must cast to a simulator-specific port type");
        std::vector<T*> ports;
        for (const auto& p : m_specialPorts) {
            ports.push_back(p.second);
        }
        return ports;
    }

    void setSpecialPort(const std::string& id, SimPort* port) {
        verifyHasSpecialPortID(id);
        if (getSpecialPort(id) != nullptr)
            throwError("Special port '" + id + "' already set");
        m_specialPorts[id] = port;
    }

    template <typename T = SimComponent, typename P = NoPredicate>
    std::vector<T*> getSubComponents(const P& predicate = {}) const {
        static_assert(std::is_base_of<SimComponent, T>::value, "Must cast to a simulator-specific component type");
        std::vector<T*> subcomponents;
        for (const auto& c : m_subcomponents) {
            if constexpr (!std::is_same<NoPredicate, P>::value) {
                if (!predicate(*c))
                    continue;
            }
            subcomponents.push_back(c->cast<T>());
        }
        return subcomponents;
    }

    bool hasSubcomponents() const { return m_subcomponents.size() != 0; }

    template <typename T = SimComponent>
    void getComponentGraph(std::map<T*, std::vector<T*>>& componentGraph) {
        // Register adjacent components (child components) in the graph, and add subcomponents to graph
        componentGraph[this->cast<T>()];
        for (const auto& c : getSubComponents<T>()) {
            componentGraph[this->cast<T>()].push_back(c);
            c->getComponentGraph(componentGraph);
        }
    }

    // Component object generator that registers objects in parent upon creation
    template <typename T, typename... Args>
    T* create_component(std::string name, Args... args) {
        verifyIsUniqueComponentName(name);
        auto sptr = std::make_unique<T>(name, this, args...);
        auto* ptr = sptr.get();
        m_subcomponents.emplace(std::move(sptr));
        return ptr->template cast<T>();
    }

    template <typename T, typename... Args>
    std::vector<T*> create_components(std::string name, unsigned int n, Args... args) {
        std::vector<T*> components;
        for (unsigned int i = 0; i < n; i++) {
            std::string i_name = name + "_" + std::to_string(i);
            components.push_back(create_component<T, Args...>(i_name, args...));
        }
        return components;
    }

    template <typename T>
    Parameter<T>& createParameter(std::string name, const T& value) {
        verifyIsUniqueParameterName(name);
        auto sptr = std::make_unique<Parameter<T>>(name, value);
        auto* ptr = sptr.get();
        m_parameters.emplace(std::move(sptr));
        return *ptr;
    }

    std::vector<ParameterBase*> getParameters() const {
        std::vector<ParameterBase*> parameters;
        for (const auto& p : m_parameters) {
            parameters.push_back(p.get());
        }
        return parameters;
    }

    void verifyIsUniqueComponentName(const std::string& name) {
        if (!isUniqueName(name, m_subcomponents)) {
            throw std::runtime_error("Duplicate subcomponent name: '" + name + "' in component: '" + getName() +
                                     "'. Subcomponent names must be unique.");
        }
    }

    void verifyIsUniqueParameterName(const std::string& name) {
        if (!isUniqueName(name, m_parameters)) {
            throw std::runtime_error("Duplicate parameter name: '" + name + "' in component: '" + getName() +
                                     "'. Parameter names must be unique.");
        }
    }

    template <typename T, typename C_T>
    bool isUniqueName(const std::string& name, std::set<std::unique_ptr<T>, C_T>& container) {
        return std::find_if(container.begin(), container.end(),
                            [name](const auto& p) { return p->getName() == name; }) == container.end();
    }

    template <typename T>
    T* cast() {
        static_assert(std::is_base_of<SimComponent, T>::value, "Must cast to a simulator-specific component type");
        if constexpr (std::is_same<T, SimComponent>::value) {
            return this;
        } else {
            return dynamic_cast<T*>(this);
        }
    }
    unsigned reserveConstantId() { return m_constantCount++; }

    void registerSynchronous(SimSynchronous* s) {
        if (m_synchronous != nullptr) {
            throw std::runtime_error("A synchronous object has already been registered to this component");
        }
        m_synchronous = s;
    }
    bool isSynchronous() const { return m_synchronous != nullptr; }
    SimSynchronous* getSynchronous() { return m_synchronous; }

    Gallant::Signal0<> changed;

protected:
    // Ports and subcomponents should be maintained as sorted sets based on port and component names, ensuring
    // consistent ordering between executions
    std::set<std::unique_ptr<SimPort>, PortBaseCompT> m_outputPorts;
    std::set<std::unique_ptr<SimPort>, PortBaseCompT> m_inputPorts;
    std::set<std::unique_ptr<SimComponent>, ComponentCompT> m_subcomponents;
    std::set<std::unique_ptr<ParameterBase>> m_parameters;
    std::map<std::string, SimPort*> m_specialPorts;

private:
    unsigned m_constantCount = 0;  // Number of constants currently initialized in the component
    SimSynchronous* m_synchronous = nullptr;
};

/**
 * @brief The SimSynchronous class
 * Seen as addition to the interface of the SimComponent class, but not through inheritance. This is due to the fact
 * that simulators may have their synchronous components as inheriting from the simulator-specific component type.
 * To avoid a constraint saying that all interface and simulator level inheritance declarations must be declared
 * virtual, we define synchronous elements as being an optional part which may be included in a normal SimComponent.
 */
class SimSynchronous {
public:
    SimSynchronous(SimComponent* parent) : m_parent(parent) { m_parent->registerSynchronous(this); }
    virtual ~SimSynchronous() {}
    virtual void reset() = 0;
    virtual void reverse() = 0;
    virtual void forceValue(VSRTL_VT_U addr, VSRTL_VT_U value) = 0;

private:
    /**
     * @brief m_parent
     * Defined as the object of which this synchronous declaration extends
     */
    SimComponent* m_parent = nullptr;
};

class SimDesign : public SimComponent {
public:
    SimDesign(std::string name, SimBase* parent) : SimComponent(name, parent) {}
    virtual ~SimDesign() {}
    /**
     * @brief clock
     * Simulates clocking the circuit. Registers are clocked and the propagation algorithm is run
     * @pre A call to propagate() must be done, to set the initial state of the circuit
     */
    virtual void clock() {
#ifndef NDEBUG
        assert(m_cycleCount != m_cycleCountPre && "Sim library should update cycle count!");
        m_cycleCountPre = m_cycleCount;
#endif

        if (clockedSignalsEnabled()) {
            designWasClocked.Emit();
        }
    }

    /**
     * @brief reverse
     * Undo the last clock operation. Registers will assert their previous state value. Memory elements will undo
     * their last transaction. The circuit shall be repropagated and assume its previous-cycle state.
     */
    virtual void reverse() {
#ifndef NDEBUG
        assert(m_cycleCount != m_cycleCountPre && "Sim library should update cycle count!");
        m_cycleCountPre = m_cycleCount;
#endif
        if (clockedSignalsEnabled()) {
            designWasReversed.Emit();
        }
    }

    /**
     * @brief propagate
     * Propagate the circuit.
     */
    virtual void propagate() = 0;

    /**
     * @brief reset
     * Resets the circuit, setting all registers to 0 and propagates the circuit. Constants might have an affect on
     * the circuit in terms of not all component values being 0.
     */
    virtual void reset() {
#ifndef NDEBUG
        assert(m_cycleCount == 0 && "Sim library should have reset cycle count!");
        m_cycleCountPre = -1;
#endif
        if (clockedSignalsEnabled()) {
            designWasReset.Emit();
        }
    }

    /**
     * @brief canReverse
     * @return is the simulator able to reverse?
     */
    virtual bool canReverse() const = 0;

    /**
     * @brief verifyAndInitialize
     * Any post-construction initialization should be included in this function.
     */
    virtual void verifyAndInitialize() = 0;

    /**
     * m_emitsSignals related functions
     * signalsEnabled() may be used by child components and ports of this design, to emit status change signals.
     */
    void setEnableSignals(bool state) { m_emitsSignals = state; }
    bool signalsEnabled() const { return m_emitsSignals; }

    /**
     * m_emitsClockedSignals related functions
     * clockedSignalsEnabled() may be used by the design to control whether the clocked/reversed signals are emitted.
     */
    void setEnableClockedSignals(bool state) { m_emitsClockedSignals = state; }
    bool clockedSignalsEnabled() const { return m_emitsClockedSignals; }

    virtual std::vector<SimComponent*> getRegisters() const {
        return getSubComponents([=](SimComponent& c) { return c.isSynchronous(); });
    }

    long long getCycleCount() const { return m_cycleCount; }

    virtual void setSynchronousValue(SimSynchronous* c, VSRTL_VT_U addr, VSRTL_VT_U value) = 0;

    /**
     * @brief clocked, reversed & reset signals
     * These signals are emitted whenever the design has finished an entire clockcycle (clock + signal propagation).
     * Signals are emitted if m_emitsClockedSignals is set.
     */
    Gallant::Signal0<> designWasClocked;
    Gallant::Signal0<> designWasReversed;
    Gallant::Signal0<> designWasReset;

protected:
    long long m_cycleCount = 0;
    bool m_emitsSignals = true;

private:
    bool m_emitsClockedSignals = true;

#ifndef NDEBUG
    long long m_cycleCountPre = 0;
#endif
};

}  // namespace vsrtl
