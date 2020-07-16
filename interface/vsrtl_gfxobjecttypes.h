#pragma once

#include <algorithm>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <vector>

namespace vsrtl {

// @todo: describe why this is needed (graphical objects need not know about the templated versions of objects)

class GraphicsType {
public:
    bool hasSpecialPortID(const std::string& id) const {
        const auto& v = specialPortIDs();
        return std::find(v.begin(), v.end(), id) != v.end();
    }
    virtual const std::vector<std::string> specialPortIDs() const = 0;
    virtual std::string getName() const = 0;
    virtual ~GraphicsType() {}
};

#define GraphycsTypeForComponent(name) name##GraphicsType

#define DefineGraphicsType(type, requiredSpecialPorts)                                                             \
    class GraphycsTypeForComponent(type) : public GraphicsType {                                                   \
    public:                                                                                                        \
        static std::type_index getGraphicsID() { return std::type_index(typeid(GraphycsTypeForComponent(type))); } \
        const std::vector<std::string> specialPortIDs() const override { return requiredSpecialPorts; }            \
        static GraphicsType* get() {                                                                               \
            static GraphycsTypeForComponent(type) instance;                                                        \
            return &instance;                                                                                      \
        }                                                                                                          \
        std::string getName() const override { return #type; }                                                     \
                                                                                                                   \
    private:                                                                                                       \
        GraphycsTypeForComponent(type)() {}                                                                        \
    }

#define GraphicsIDFor(type) (GraphycsTypeForComponent(type)::getGraphicsID())

// All simulator components should use the following macro for defining how the component type should be drawn. Any of
// the supported objects listed below may be specified.
#define SetGraphicsType(name)                                                      \
    std::type_index getGraphicsID() const override { return GraphicsIDFor(name); } \
    const GraphicsType* getGraphicsType() const override { return GraphycsTypeForComponent(name)::get(); }

#define L(...) __VA_ARGS__

// Supported graphical objects
DefineGraphicsType(Component, {});

DefineGraphicsType(Register, {});

DefineGraphicsType(Wire, {});

DefineGraphicsType(Constant, {});

DefineGraphicsType(ClockedComponent, {});

DefineGraphicsType(And, {});

DefineGraphicsType(Or, {});

DefineGraphicsType(Xor, {});

DefineGraphicsType(Not, {});

DefineGraphicsType(Multiplexer, {"select"});

DefineGraphicsType(ALU, {});

DefineGraphicsType(Adder, {});

}  // namespace vsrtl
