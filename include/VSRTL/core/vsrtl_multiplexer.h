#ifndef VSRTL_MULTIPLEXER_H
#define VSRTL_MULTIPLEXER_H

#include "vsrtl_component.h"
#include "vsrtl_defines.h"
#include "vsrtl_enum.h"
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
    out << [=] { return ins.at(select.uValue())->uValue(); };
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
    setSpecialPort(GFX_MUX_SELECT, &select);
    for (auto v : E_t::_values()) {
      m_enumToPort[v] = this->ins.at(v);
    }
    out << [=] { return ins.at(select.uValue())->uValue(); };
  }

  Port<W> &get(unsigned enumIdx) {
    if (m_enumToPort.count(enumIdx) != 1) {
      throw std::runtime_error("Requested index out of Enum range");
    }
    if (m_enumToPort[enumIdx] == nullptr) {
      throw std::runtime_error(
          "Requested enum index not associated with any port");
    }
    return *m_enumToPort[enumIdx];
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
  INPUTPORTS(ins, W, E_t::_size());

private:
  std::map<int, Port<W> *> m_enumToPort;
};

} // namespace core
} // namespace vsrtl

#endif // VSRTL_MULTIPLEXER_H
