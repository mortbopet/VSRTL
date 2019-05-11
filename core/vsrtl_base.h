#ifndef VSRTL_BASE_H
#define VSRTL_BASE_H

#include <stdexcept>

namespace vsrtl {

/**
 * @brief The Base class
 * Base class for any vsrtl object which may have a high-level counterpart in the graphics or EDA library which index
 * into the vsrtl circuit graph. void* are used given that the registered graphical object should be generic, but Base
 * should not be a templated type.
 */
class Base {
public:
    Base() {}
    void registerSuper(void* obj) {
        if (m_superObject != nullptr) {
            throw std::runtime_error("Super object already registered");
        }
        m_superObject = obj;
    }
    void* getSuper() const {
        if (m_superObject == nullptr) {
            throw std::runtime_error("Super object not registered");
        }
        return m_superObject;
    }

    virtual ~Base() {}

private:
    void* m_superObject = nullptr;
};
}  // namespace vsrtl

#endif  // VSRTL_BASE_H
