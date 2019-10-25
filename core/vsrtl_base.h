#ifndef VSRTL_BASE_H
#define VSRTL_BASE_H

#include <stdexcept>

namespace vsrtl {

/**
 * @brief The Base class
 * Base class for any vsrtl object which may have a graphical counterpart in a graphics library which index into the
 * vsrtl circuit graph.
 * void* are used given that the registered graphical object should be generic, but Base should not be a templated type.
 *
 */

class Component;

class Base {
public:
    Base(std::string name, Component* parent) : m_name(name), m_parent(parent) {}

    Component* getParent() const { return m_parent; }
    const std::string& getName() const { return m_name; }

    void registerGraphic(void* obj) {
        if (m_graphicObject != nullptr) {
            throw std::runtime_error("Graphic object already registered");
        }
        m_graphicObject = obj;
    }
    void* getGraphic() const {
        if (m_graphicObject == nullptr) {
            throw std::runtime_error("Graphic object not registered");
        }
        return m_graphicObject;
    }

    virtual ~Base() {}

private:
    Component* m_parent = nullptr;
    std::string m_name;
    void* m_graphicObject = nullptr;
};
}  // namespace vsrtl

#endif  // VSRTL_BASE_H
