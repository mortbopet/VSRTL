#pragma once

#include "../interface/vsrtl_defines.h"
#include "../interface/vsrtl_interface.h"

#include "vlt_component.h"

#include "pugixml.hpp"

#include <variant>

namespace vsrtl {
namespace vlt {

class Component;
class Port;

class Design : public SimDesign {
public:
    Design(const std::string& xmlPath);

    /**
     * @brief clock
     * Simulates clocking the circuit. Registers are clocked and the propagation algorithm is run
     * @pre A call to propagate() must be done, to set the initial state of the circuit
     */
    void clock() override;
    void reverse() override { throw std::runtime_error("Cannot reverse."); }
    void propagate() override { /* Todo: circuit->eval() */
    }

    /**
     * @brief reset
     * Resets the circuit, setting all registers to 0 and propagates the circuit. Constants might have an affect on the
     * circuit in terms of not all component values being 0.
     */
    void reset() override { /* Todo: circuit->eval() */
    }

    bool canReverse() const override { return false; }

    void setSynchronousValue(SimSynchronous* c, VSRTL_VT_U addr, VSRTL_VT_U value) override {
        c->forceValue(addr, value);
        propagate();
    }

    /**
     * @brief verifyAndInitialize
     * Calls verifyDesign() to ensure that all the required inputs for each initialized object have been set, and
     * propagates the circuit to set the initial state.
     */
    void verifyAndInitialize() override { reset(); }

private:
    using TypeID = unsigned;
    struct Type {
        int left = 0;
        int right = 0;
        std::string name;
        int width() const { return left - right + 1; }
    };
    unsigned typeWidth(const TypeID& typeID) const;

    void throwError(const std::string& err) const;
    void throwWarning(const std::string& err) const;
    int nextModuleId(const std::string& modName);

    const std::string m_xmlPath;
    std::map<TypeID, Type> m_types;

    // XML loading functions
    void loadDesign();
    void loadTypeTable(const pugi::xml_node& typetable);
    Component* loadInstance(Component* parent, const pugi::xml_node& mod, const std::string& instanceName = {});
    void loadVar(Component* comp, const pugi::xml_node& pin);
    void loadAssignments(Component* comp, const pugi::xml_node& mod);
    void loadContAssign(Component* comp, const pugi::xml_node& contassign);

    Port* loadExpr(Component* parent, const pugi::xml_node& expr);
    Port* loadExprErroring(Component* parent, const pugi::xml_node& expr);
    pugi::xml_node findModuleNode(const std::string& name);
    pugi::xml_node m_netlist;
};

}  // namespace vlt

}  // namespace vsrtl
