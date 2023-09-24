#pragma once

#include <string>
#include <vector>

#include "Signal.h"

namespace vsrtl {

class ParameterBase {
public:
  ParameterBase(const std::string &name) : m_name(name) {}
  virtual ~ParameterBase() {}
  const std::string &getName() const { return m_name; }
  const std::string &getTooltip() const { return m_tooltip; }
  void setTooltip(const std::string &tooltip) { m_tooltip = tooltip; }

protected:
  std::string m_name;
  std::string m_tooltip;
};

/**
 * @brief The Parameter class
 * If the parameter of type @p T is supported by the GUI, the parameter may be
 * used in the component through the get/set functions to retrieve values as set
 * through the GUI. Alternate type @p IT is provided for specifying the initial
 * value of the parameter, in cases where parameter type and initial type differ
 * (such as if a parameter is of type List<String>, but has an initial value of
 * type String.
 */
template <typename T>
class Parameter : public ParameterBase {
  static_assert(std::is_same<std::string, T>::value ||
                    std::is_same<bool, T>::value ||
                    std::is_same<int, T>::value ||
                    std::is_same<std::vector<int>, T>::value ||
                    std::is_same<std::vector<std::string>, T>::value,
                "Unsupported parameter type");

public:
  Parameter(const std::string &name, const T &value = T())
      : ParameterBase(name), m_value(value) {}
  T &getValue() { return m_value; }
  void setValue(const T &value) {
    m_value = value;
    changed.Emit();
  }

  /**
   * @brief Option semantics
   * T=int            : Value range = [options[0], options[1]]
   * Any vector type  : Allowed values
   */
  void setOptions(const std::vector<T> &options) { m_options = options; }
  const std::vector<T> &getOptions() const { return m_options; }

  Gallant::Signal0<> changed;

protected:
  T m_value;
  std::vector<T> m_options;
};

} // namespace vsrtl
