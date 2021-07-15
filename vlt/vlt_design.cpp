#include "vlt_design.h"

#include "pugixml.hpp"

#include "vlt_component.h"
#include "vlt_port.h"

/* Verilator XML format identifiers */
constexpr auto VLT_ROOT = "verilator_xml";
constexpr auto VLT_DIR = "dir";
constexpr auto VLT_NETLIST = "netlist";
constexpr auto VLT_MODULE = "module";
constexpr auto VLT_MODULE_NAME = "name";
constexpr auto VLT_INSTANCE = "instance";
constexpr auto VLT_INSTANCE_NAME = "name";
constexpr auto VLT_PORT_NAME = "name";
constexpr auto VLT_INSTANCE_DEFNAME = "defName";
constexpr auto VLT_TOPMODULE = "topModule";
constexpr auto VLT_VAR = "var";
constexpr auto VLT_TYPETABLE = "typetable";

/**********************************************************/

namespace vsrtl {
namespace vlt {

Design::Design(const std::string& xmlPath) : SimDesign(xmlPath, nullptr), m_xmlPath(xmlPath) {
    loadDesign();
}

void Design::throwError(const std::string& err) const {
    throw std::runtime_error("ERROR: " + err);
}

void Design::throwWarning(const std::string& err) const {
    std::cout << "ERROR: " << err << std::endl;
}

unsigned Design::typeWidth(const TypeID& typeID) const {
    auto it = m_types.find(typeID);
    if (it == m_types.end()) {
        // throwError("No type registerred for typeid " + std::to_string(typeID) + "'");
        // @todo: This will occur for anything but the types that are currently loaded from loadTypeTable. Just return
        // some dummy value for now
        return 43;
    }
    return it->second.width();
}

void Design::loadTypeTable(const pugi::xml_node& typetable) {
    // Only support basic types for now
    for (auto vlttype : typetable.children()) {
        const std::string typeName = std::string(vlttype.name());
        if (typeName == "basicdtype") {
            Type type;

            type.name = vlttype.attribute("name").as_string();
            type.left = vlttype.attribute("left").as_int(0);
            type.right = vlttype.attribute("right").as_int(0);
            m_types[vlttype.attribute("id").as_int()] = type;
        } else {
            throwWarning("Unhandled type '" + typeName + "'");
        }
    }
}

void Design::loadVar(Component* comp, const pugi::xml_node& pin) {
    const auto pinName = pin.attribute(VLT_PORT_NAME).as_string();
    const auto typeId = pin.attribute("dtype_id").as_uint();
    const auto dir = std::string(pin.attribute(VLT_DIR).as_string());
    const auto varType = std::string(pin.attribute("vartype").as_string());
    const auto& width = typeWidth(typeId);
    if (dir == "input") {
        comp->createInputPort(pinName, width);
    } else if (dir == "output") {
        comp->createOutputPort(pinName, width);
    } else if (varType == "logic") {
        comp->createSignal(pinName, width);
    }
}

pugi::xml_node Design::findModuleNode(const std::string& name) {
    auto node = m_netlist.find_child_by_attribute(VLT_MODULE, VLT_MODULE_NAME, name.c_str());
    if (!node) {
        throwError("Could not find module named: " + name);
    }

    return node;
}

/**
 * @brief Design::loadModule
 * @param parent
 * @param mod
 * @param instanceName
 *
 * Three components are in flight during loadInstance:
 * - parent: The parent component which instantiated the component that is currently being instantiated
 * - component: The component currently being instantiated
 * - subcomponent: components being instantiated by the currently instantiating component
 *
 * @note: This could be done more cleverly, i.e., a module should only be created once, and then copied when
 * instantiated.
 *
 */
Component* Design::loadInstance(Component* parent, const pugi::xml_node& mod, const std::string& instanceName) {
    const auto modName = instanceName.empty() ? mod.attribute(VLT_MODULE_NAME).as_string() : instanceName;

    Component* component = nullptr;
    if (!parent) {
        component = this->create_component<Component>(modName);
    } else {
        component = parent->create_component<Component>(modName);
    }

    // Load vars
    for (pugi::xml_node var : mod.children(VLT_VAR)) {
        loadVar(component, var);
    }

    // Load instantiated entities
    for (pugi::xml_node instance : mod.children(VLT_INSTANCE)) {
        const auto instName = instance.attribute(VLT_INSTANCE_NAME).as_string();
        const auto defName = instance.attribute(VLT_INSTANCE_DEFNAME).as_string();
        // const auto defId = nextModuleId(defName);
        auto* subComponent = loadInstance(component, findModuleNode(defName), instName);

        // Load port assignments to instanciated entity
        for (pugi::xml_node port : instance.children("port")) {
            const auto direction = std::string(port.attribute("direction").as_string());
            auto* subCompPort = subComponent->findPort<Port>(port.attribute("name").as_string());
            if (!subCompPort) {
                std::cout << "Ports for " << subComponent->getHierName() << " were: " << std::endl;
                for (const auto& p : subComponent->getAllPorts()) {
                    std::cout << "\t" << p->getHierName() << std::endl;
                }
            }
            if (port.children().empty()) {
                // Port is unassigned
                continue;
            }

            auto* assignedVar = loadExprErroring(component, *port.children().begin());
            if (subCompPort && assignedVar) {
                if (direction == "in") {
                    *assignedVar >> *subCompPort;
                } else if (direction == "out") {
                    *subCompPort >> *assignedVar;
                } else {
                    throw std::runtime_error("Unknown direction for port");
                }
            }
        }
    }

    // Load assignments
    loadAssignments(component, mod);

    return component;
}

/* Conversion between VLT expression identifiers and the named graphics type of VSRTL. These are generally the same,
 * but a mapping is maintained since they are logically referring to different things */
static const std::map<std::string, std::string> c_vltToOpType = {{"and", "and"},
                                                                 {"or", "or"},
                                                                 {"not", "not"},
                                                                 {"xor", "xor"},
                                                                 {"sel", "multiplexer"}};
class Op : public Component {
public:
    Op(const std::string& name, SimComponent* parent, const GraphicsType* graphicsType)
        : Component(name, parent), m_graphicsType(graphicsType) {
        assert(graphicsType != nullptr);
    }
    const GraphicsType* getGraphicsType() const override { return m_graphicsType; }

private:
    const GraphicsType* m_graphicsType;
};

class Constant : public Component {
public:
    SetGraphicsType(Constant);
    Constant(const std::string& name, SimComponent* parent, unsigned width, VSRTL_VT_U value = 0)
        : Component(name, parent) {
        output = &createOutputPort("out", width);
        output->setConstant(true, value);
    }

    Port* output;
};

Port* Design::loadExprErroring(Component* parent, const pugi::xml_node& expr) {
    const std::string exprName = std::string(expr.attribute("name").as_string());

    auto* port = loadExpr(parent, expr);
    if (!port) {
        throw std::runtime_error("Could not find port or signal '" + exprName + "' in component '" + getHierName() +
                                 "'");
    }
    return port;
}

Port* Design::loadExpr(Component* parent, const pugi::xml_node& expr) {
    const std::string exprType = std::string(expr.name());
    const std::string exprName = std::string(expr.attribute("name").as_string());

    if (exprType == "varref") {
        if (auto* ioPort = parent->findPort<Port>(exprName)) {
            return ioPort;
        } else if (auto* signal = parent->findSignal<Port>(exprName)) {
            return signal;
        }
    } else if (exprType == "const") {
        auto* constant_cmp = parent->create_component<Constant>(parent->addSuffix(expr.attribute("name").as_string()),
                                                                typeWidth(expr.attribute("dtype_id").as_uint()));
        return constant_cmp->output;
    } else {
        // General operations with some in- and outputs

        GraphicsType const* gfxType = nullptr;
        auto gfxTypeIt = c_vltToOpType.find(exprType);
        if (gfxTypeIt != c_vltToOpType.end()) {
            gfxType = GraphicsTypeFromName::get(gfxTypeIt->second);
        } else {
            // Default - just show as a component
            gfxType = GraphicsTypeFromName::get("component");
        }

        auto* op = parent->create_component<Op>(parent->addSuffix(exprType), gfxType);
        const unsigned width = typeWidth(expr.attribute("dtype_id").as_uint());
        int i = 0;
        // For each input, create a new port in the op component, and connect the port to the result of further
        // evaluating the expression
        for (auto child : expr.children()) {
            auto& port = op->createInputPort("in" + std::to_string(i), width);
            auto* fromPort = loadExprErroring(parent, child);
            if (fromPort) {
                *fromPort >> port;
            }
            i++;
        }

        // Multiplexers
        if (exprType == "sel") {
            // Select signal is expected to be the 1st signal under the "sel" tag.
            op->setSpecialPort(GFX_MUX_SELECT, op->getInputPorts().at(0));
        }

        // Output port
        return &op->createOutputPort("", width);
    }

    return nullptr;
}

void Design::loadContAssign(Component* comp, const pugi::xml_node& contassign) {
    pugi::xml_node from, to;
    int i = 0;

    Port* fromPort = nullptr;
    Port* toPort = nullptr;

    for (auto node : contassign.children()) {
        if (i == 0) {
            from = node;
        } else {
            to = node;
        }
        i++;
    }

    fromPort = loadExprErroring(comp, from);
    toPort = loadExprErroring(comp, to);
    *fromPort >> *toPort;
}

void Design::loadAssignments(Component* comp, const pugi::xml_node& mod) {
    for (auto contassign : mod.children("contassign")) {
        loadContAssign(comp, contassign);
    }
}

void Design::loadDesign() {
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(m_xmlPath.c_str());
    if (!result) {
        throwError(result.description());
    }

    m_netlist = doc.child(VLT_ROOT).child(VLT_NETLIST);
    loadTypeTable(m_netlist.child(VLT_TYPETABLE));
    loadInstance(nullptr, m_netlist.find_child_by_attribute(VLT_MODULE, VLT_TOPMODULE, "1"));
}

void Design::clock() {
    SimDesign::clock();
}

}  // namespace vlt

}  // namespace vsrtl
