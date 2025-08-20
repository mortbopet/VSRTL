#ifndef VSRTL_MULTIPLEXER_H
#define VSRTL_MULTIPLEXER_H

#include "VSRTL/core/vsrtl_component.h"
#include "VSRTL/interface/vsrtl_defines.h"
#include <array>

namespace vsrtl {
namespace core {

class MultiplexerBase : public Component {
public:
  SetGraphicsType(Multiplexer);
  MultiplexerBase(const std::string &name, SimComponent *parent)
      : Component(name, parent) {}

  virtual std::vector<PortBase *> getIns() = 0;
  virtual PortBase *getSelect() = 0;
  virtual PortBase *getOut() = 0;
};

template <unsigned int N, unsigned int W>
class Multiplexer : public MultiplexerBase {
public:
  Multiplexer(const std::string &name, SimComponent *parent)
      : MultiplexerBase(name, parent) {
    setSpecialPort(GFX_MUX_SELECT, &select);
    out << [this] { return ins.at(select.uValue())->uValue(); };
  }

  std::vector<PortBase *> getIns() override {
    std::vector<PortBase *> ins_base;
    for (const auto &in : ins)
      ins_base.push_back(in);
    return ins_base;
  }

  virtual Port<W> &get(unsigned idx) {
    if (idx >= ins.size()) {
      throw std::runtime_error("Requested index out of multiplexer range");
    }
    return *ins[idx];
  }

  /**
   * @brief others
   * @return a vector of all ports which has not been connected
   */
  std::vector<Port<W> *> others() {
    std::vector<Port<W> *> vec;
    for (const auto &port : ins) {
      if (!port->getInputPort()) {
        vec.push_back(port);
      }
    }
    return vec;
  }

  PortBase *getSelect() override { return &select; }
  PortBase *getOut() override { return &out; }

  OUTPUTPORT(out, W);
  INPUTPORT(select, ceillog2(N));
  INPUTPORTS(ins, W, N);
};

/** @class EnumMultiplexer
 * A multiplexer which may be initialized with a VSRTL Enum.
 * The select signal and number of input ports will be inferred based on the
 * enum type.
 *
 */
template <typename E_t, unsigned W>
class EnumMultiplexer : public MultiplexerBase {
public:
  EnumMultiplexer(const std::string &name, SimComponent *parent)
      : MultiplexerBase(name, parent) {
    if (this->ins.size() != magic_enum::enum_count<E_t>()) {
      throw std::runtime_error(
          "EnumMultiplexer: Number of input ports does not "
          "match number of enum entries");
    }

    setSpecialPort(GFX_MUX_SELECT, &select);
    for (auto v : magic_enum::enum_entries<E_t>()) {
      m_enumToPort[v.first] =
          this->ins.at(magic_enum::enum_index<E_t>(v.first).value());
    }
    out << [this] { return ins.at(select.uValue())->uValue(); };
  }

  Port<W> &get(E_t e) {
    auto it = m_enumToPort.find(e);
    if (it == m_enumToPort.end()) {
      throw std::runtime_error("Requested enum index not found");
    }
    return *it->second;
  }

  Port<W> &get(unsigned enumIdx) {
    constexpr size_t enumSize = magic_enum::enum_count<E_t>();
    if (enumIdx >= enumSize) {
      throw std::runtime_error("Requested enum index out of range");
    }
    auto e = magic_enum::enum_value<E_t>(enumIdx);
    return get(e);
  }

  std::vector<PortBase *> getIns() override {
    std::vector<PortBase *> ins_base;
    for (const auto &in : ins)
      ins_base.push_back(in);
    return ins_base;
  }

  std::vector<Port<W> *> others() {
    std::vector<Port<W> *> vec;
    for (const auto &port : ins) {
      if (!port->getInputPort()) {
        vec.push_back(port);
      }
    }
    return vec;
  }

  PortBase *getSelect() override { return &select; }
  PortBase *getOut() override { return &out; }

  OUTPUTPORT(out, W);
  INPUTPORT_ENUM(select, E_t);
  INPUTPORTS(ins, W, magic_enum::enum_count<E_t>());

private:
  std::map<E_t, Port<W> *> m_enumToPort;
};

} // namespace core
} // namespace vsrtl

#endif // VSRTL_MULTIPLEXER_H
