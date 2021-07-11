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

void Design::loadPin(Component* comp, const pugi::xml_node& pin) {
    const auto dir = std::string(pin.attribute(VLT_DIR).as_string());
    const auto pinName = pin.attribute(VLT_PORT_NAME).as_string();
    const auto typeId = pin.attribute("dtype_id").as_uint();
    const auto& width = typeWidth(typeId);
    if (dir == "input") {
        comp->createInputPort(pinName, width);
    } else if (dir == "output") {
        comp->createOutputPort(pinName, width);
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
 * @note: This could be done more cleverly, i.e., a module should only be created once, and then copied when
 * instantiated.
 */
void Design::loadModule(Component* parent, const pugi::xml_node& mod, const std::string& instanceName) {
    const auto modName = instanceName.empty() ? mod.attribute(VLT_MODULE_NAME).as_string() : instanceName;

    Component* component = nullptr;
    if (!parent) {
        component = this->create_component<Component>(instanceName);
    } else {
        component = parent->create_component<Component>(instanceName);
    }

    // Load pins
    for (pugi::xml_node var : mod.children(VLT_VAR)) {
        if (var.attribute(VLT_DIR)) {
            loadPin(component, var);
        }
    }

    // Load instantiated entities
    for (pugi::xml_node instance : mod.children(VLT_INSTANCE)) {
        const auto instName = instance.attribute(VLT_INSTANCE_NAME).as_string();
        const auto defName = instance.attribute(VLT_INSTANCE_DEFNAME).as_string();
        // const auto defId = nextModuleId(defName);
        loadModule(component, findModuleNode(defName), instName);
    }

    // Load assignments
    loadAssignments(component, mod);
}

/* Conversion between VLT expression identifiers and the named graphics type of VSRTL. These are generally the same,
 * but a mapping is maintained since they are logically referring to different things */
static const std::map<std::string, std::string> c_vltToOpType = {{"and", "and"},
                                                                 {"or", "or"},
                                                                 {"not", "not"},
                                                                 {"xor", "xor"}};
class Op : public Component {
public:
    Op(const std::string& name, SimComponent* parent, const GraphicsType* graphicsType)
        : Component(name, parent), m_graphicsType(graphicsType) {}
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

Port* Design::loadExpr(Component* parent, const pugi::xml_node& expr) {
    const std::string exprType = std::string(expr.name());

    if (exprType == "varref") {
        return findPort<Port>(expr.attribute("name").as_string());
    } else if (c_vltToOpType.count(exprType)) {
        auto* op = parent->create_component<Op>(parent->addSuffix(exprType), GraphicsTypeFromName::get(exprType));
        int i = 0;
        // For each input, create a new port in the op component, and connect the port to the result of further
        // evaluating the expression
        for (auto child : expr.children()) {
            auto& port = op->createInputPort("in" + std::to_string(i), typeWidth(expr.attribute("dtype_id").as_uint()));
            auto* fromPort = loadExpr(parent, child);
            if (fromPort) {
                *fromPort >> port;
            }
            i++;
        }
    } else if (exprType == "const") {
        auto* constant_cmp = parent->create_component<Constant>(parent->addSuffix(expr.attribute("name").as_string()),
                                                                typeWidth(expr.attribute("dtype_id").as_uint()));
        return constant_cmp->output;

    }

    else {
        throwWarning("Unhandled expression type '" + exprType + "'");
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

    fromPort = loadExpr(comp, from);
    toPort = loadExpr(comp, to);
    if (fromPort && toPort) {
        *fromPort >> *toPort;
    }
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
    loadModule(nullptr, m_netlist.find_child_by_attribute(VLT_MODULE, VLT_TOPMODULE, "1"));
}

void Design::clock() {
    SimDesign::clock();
}

}  // namespace vlt

}  // namespace vsrtl
