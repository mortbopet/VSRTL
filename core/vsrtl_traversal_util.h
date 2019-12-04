#ifndef VSRTL_TRAVERSAL_UTIL_H
#define VSRTL_TRAVERSAL_UTIL_H

// Graph traversal utility functions for casting the void* registered graphics to the graphics library specified types

#include "vsrtl_base.h"
#include "vsrtl_port.h"

namespace vsrtl {
namespace core {

template <typename T>
T getGraphic(Base* b) {
    return static_cast<T>(b->getGraphic());
}

template <typename T>
T getInputPortGraphic(PortBase* p) {
    return getGraphic<T>(p->getInputPort());
}

template <typename T>
std::vector<T> getOutputPortGraphics(PortBase* p) {
    std::vector<T> portGraphics;
    for (const auto& out : p->getOutputPorts()) {
        portGraphics.push_back(getGraphic<T>(out));
    }
    return portGraphics;
}

}  // namespace core
}  // namespace vsrtl

#endif  // VSRTL_TRAVERSAL_UTIL_H
