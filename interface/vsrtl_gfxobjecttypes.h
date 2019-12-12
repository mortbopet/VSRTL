#pragma once

#include <typeindex>
#include <typeinfo>

namespace vsrtl {

// @todo: describe why this is needed (graphical objects need not know about the templated versions of objects)
#define DefineGraphicsType(name)                                                                         \
    class name##GraphicsProxy {                                                                          \
    public:                                                                                              \
        name##GraphicsProxy() {}                                                                         \
        static std::type_index getTypeIdProxy() { return std::type_index(typeid(name##GraphicsProxy)); } \
    };

#define GraphicsTypeFor(name) (name##GraphicsProxy::getTypeIdProxy())

// All simulator components should use the following macro for defining how the component type should be drawn. Any of
// the supported objects listed below may be specified.
#define SetGraphicsType(name) \
    std::type_index getTypeId() const override { return GraphicsTypeFor(name); }

// Supported graphical objects
DefineGraphicsType(Component);

// Constraints: single input and output
DefineGraphicsType(Register);

// Constraints:
DefineGraphicsType(Constant);

// Constraints:
DefineGraphicsType(ClockedComponent);

// Constraints:
DefineGraphicsType(And);

// Constraints:
DefineGraphicsType(Or);

// Constraints:
DefineGraphicsType(Xor);

// Constraints:
DefineGraphicsType(Not);

// Constraints: Input 0 must be the select signal
DefineGraphicsType(Multiplexer);

// Constraints:
DefineGraphicsType(ALU);

// Constraints:
DefineGraphicsType(Adder);

}  // namespace vsrtl
