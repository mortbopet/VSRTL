#ifndef GRIDPORT_H
#define GRIDPORT_H

#include <memory>

#include "core/vsrtl_base.h"
#include "core/vsrtl_port.h"
#include "defines.h"
#include "geometry.h"

#include <QPoint>

namespace vsrtl {
namespace eda {

struct Route;
// Redefine the Net using declarative to avoid a circular dependency from routing.h
using Net = std::vector<std::unique_ptr<Route>>;

class GridPort : public Base {
public:
    GridPort(Port& p) : m_port(p) { p.registerSuper(this); }

    void setNet(std::unique_ptr<Net>& net) { m_net = std::move(net); }
    constexpr static inline int width() { return PORT_GRID_WIDTH; }
    void setPosition(Edge edge, unsigned int offset) {
        m_edge = edge;
        m_offset = offset;
    }

    Port* port() { return &m_port; }

    std::pair<Edge, unsigned int> getPosition() const { return {m_edge, m_offset}; }

private:
    Port& m_port;
    std::unique_ptr<Net> m_net;

    // Port position in its parent grid component. Offset goes from left to right or top to bottom.

    /// @todo move this logic to parent, GridComponent
    Edge m_edge;
    unsigned int m_offset;
};

}  // namespace eda
}  // namespace vsrtl

#endif  // GRIDPORT_H
