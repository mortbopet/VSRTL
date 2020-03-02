#pragma once

#include <string>
#include <vector>

#include "../external/Signals/Signal.h"

namespace vsrtl {

class ParameterBase {
public:
    ParameterBase(const std::string& name) : m_name(name) {}
    const std::string& getName() const { return m_name; }

protected:
    std::string m_name;
};

/**
 * @brief The Parameter class
 * If the parameter of type @p T is supported by the GUI, the parameter may be used in the component through the get/set
 * functions to retrieve values as set through the GUI.
 * Alternate type @p IT is provided for specifying the initial value of the parameter, in cases where parameter type and
 * initial type differ (such as if a parameter is of type List<String>, but has an initial value of type String.
 */
template <typename T, typename IT = T>
class Parameter : public ParameterBase {
public:
    Parameter(std::string name, const IT& initial = IT()) : ParameterBase(name), m_value(initial) {}
    T& getValue() { return m_value; }
    void setValue(const std::string& value) {
        m_value = value;
        changed.Emit();
    }
    void setOptions(const std::vector<T>& options) { m_options = options; }
    const std::vector<T>& getOptions() const { return m_options; }

    Gallant::Signal0<> changed;

protected:
    T m_value;
    std::vector<T> m_options;
};

}  // namespace vsrtl
