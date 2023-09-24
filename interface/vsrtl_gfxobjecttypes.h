#pragma once

#include <algorithm>
#include <map>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <vector>

namespace vsrtl {

inline std::string str_toLower(std::string str) {
  std::transform(str.begin(), str.end(), str.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return str;
}

// @todo: describe why this is needed (graphical objects need not know about the
// templated versions of objects)

class GraphicsType;

class GraphicsTypeFromName {
public:
  static const GraphicsType *get(const std::string &name) {
    const auto name_lower = str_toLower(name);
    auto it = _get()->m_nameToType.find(name_lower);
    if (it != _get()->m_nameToType.end()) {
      return it->second;
    }
    return nullptr;
  }

  static void registerGraphicsType(const std::string &name,
                                   const GraphicsType *obj) {
    auto *instance = _get();
    const auto name_lower = str_toLower(name);
    if (instance->m_nameToType.count(name_lower) != 0) {
      throw std::runtime_error("Graphics type already registerred for type '" +
                               name + "'");
    }
    instance->m_nameToType[name_lower] = obj;
  }

private:
  GraphicsTypeFromName() {}

  static GraphicsTypeFromName *_get() {
    static GraphicsTypeFromName instance;
    return &instance;
  }

  std::map<std::string, const GraphicsType *> m_nameToType;
};

class GraphicsType {
public:
  bool hasSpecialPortID(const std::string &id) const {
    const auto &v = specialPortIDs();
    return std::find(v.begin(), v.end(), id) != v.end();
  }
  virtual const std::vector<std::string> specialPortIDs() const = 0;
  virtual std::string getName() const = 0;
  virtual ~GraphicsType() {}
};

#define GraphicsTypeFor(type) (GraphicsTypeForComponent(type)::get())

#define GraphicsTypeForComponent(name) name##GraphicsType

#define DefineGraphicsType(type, requiredSpecialPorts)                         \
  class GraphicsTypeForComponent(type)                                         \
      : public                                                                 \
        GraphicsType{public : const std::vector<std::string> specialPortIDs()  \
                         const override{return requiredSpecialPorts;           \
  }                                                                            \
  static const GraphicsType *get() {                                           \
    static GraphicsTypeForComponent(type) instance;                            \
    return &instance;                                                          \
  }                                                                            \
  std::string getName() const override { return #type; }                       \
                                                                               \
private:                                                                       \
  GraphicsTypeForComponent(type)() {                                           \
    GraphicsTypeFromName::registerGraphicsType(#type, this);                   \
  }                                                                            \
  }                                                                            \
  ;                                                                            \
  static auto *__##type##gfxtypeinstance__ =                                   \
      GraphicsTypeFor(type); // Static initializer to register type on startup

// All simulator components should use the following macro for defining how the
// component type should be drawn. Any of the supported objects listed below may
// be specified.
#define SetGraphicsType(name)                                                  \
  const GraphicsType *getGraphicsType() const override {                       \
    return GraphicsTypeForComponent(name)::get();                              \
  }

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
#define GFX_MUX_SELECT ("select")
DefineGraphicsType(Multiplexer, {GFX_MUX_SELECT});
DefineGraphicsType(ALU, {});
DefineGraphicsType(Adder, {});

} // namespace vsrtl
