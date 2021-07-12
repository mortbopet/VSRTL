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

#define GraphicsTypeForComponent(name) name##GraphicsType

#define DefineGraphicsType(type, requiredSpecialPorts)                                                  \
    class GraphicsTypeForComponent(type) : public GraphicsType {                                        \
    public:                                                                                             \
        const std::vector<std::string> specialPortIDs() const override { return requiredSpecialPorts; } \
        static const GraphicsType* get() {                                                              \
            static GraphicsTypeForComponent(type) instance;                                             \
            return &instance;                                                                           \
        }                                                                                               \
        std::string getName() const override { return #type; }                                          \
                                                                                                        \
    private:                                                                                            \
        GraphicsTypeForComponent(type)() {}                                                             \
    }

#define GraphicsTypeFor(type) (GraphicsTypeForComponent(type)::get())

// All simulator components should use the following macro for defining how the component type should be drawn. Any of
// the supported objects listed below may be specified.
#define SetGraphicsType(name) \
    const GraphicsType* getGraphicsType() const override { return GraphicsTypeForComponent(name)::get(); }

#define L(...) __VA_ARGS__

// Supported graphical objects
DefineGraphicsType(Component, {});

DefineGraphicsType(Register, {});

DefineGraphicsType(Wire, {});

DefineGraphicsType(Constant, {});

DefineGraphicsType(ClockedComponent, {});

DefineGraphicsType(And, {});

DefineGraphicsType(Nand, {});

DefineGraphicsType(Or, {});

DefineGraphicsType(Xor, {});

DefineGraphicsType(Not, {});

DefineGraphicsType(Multiplexer, {"select"});

DefineGraphicsType(ALU, {});

DefineGraphicsType(Adder, {});

}  // namespace vsrtl
