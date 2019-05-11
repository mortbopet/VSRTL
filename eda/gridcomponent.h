#ifndef GRIDCOMPONENT_H
#define GRIDCOMPONENT_H

#include <QRect>
#include <map>

#include "componentshape.h"
#include "core/vsrtl_base.h"
#include "core/vsrtl_component.h"
#include "core/vsrtl_traversal_util.h"
#include "geometry.h"
#include "gridport.h"

#include "signal.h"

namespace vsrtl {
namespace eda {

class GridComponent : public QRect, public Base {
public:
    GridComponent(Component& c) : m_component(c), m_minimumRect(getComponentMinGridRect(c.getTypeId())) {
        // Initialize port positions
        for (const auto& p : c.getInputs()) {
            m_gridPorts.push_back(std::make_unique<GridPort>(*p));
        }
        for (const auto& p : c.getOutputs()) {
            m_gridPorts.push_back(std::make_unique<GridPort>(*p));
        }

        // Register this object as the components super object
        c.registerSuper(this);
    }

    void initialize() {
        // After all GridComponents have been constructed, they are allowed to initialize, creating a link to their
        // child GridComponents
        for (const auto& c : m_component.getSubComponents()) {
            m_subcomponents.push_back(static_cast<GridComponent*>(c.get()->getSuper()));
        }
    }

    const QRect adjusted() const {
        // Returns the current rect, adjusted by the in- and output port sizes given their position
        QRect adjusted = *this;
        std::map<Edge, bool> adjustedEdge;
        adjustedEdge[Edge::Top] = false;
        adjustedEdge[Edge::Bottom] = false;
        adjustedEdge[Edge::Left] = false;
        adjustedEdge[Edge::Right] = false;
        for (const auto& port : m_gridPorts) {
            const auto edge = port->getPosition().first;
            if (!adjustedEdge[edge]) {
                switch (edge) {
                    case Edge::Top:
                        adjusted.adjust(0, -GridPort::width(), 0, 0);
                        break;
                    case Edge::Bottom:
                        adjusted.adjust(0, 0, 0, GridPort::width());
                        break;
                    case Edge::Left:
                        adjusted.adjust(-GridPort::width(), 0, 0, 0);
                        break;
                    case Edge::Right:
                        adjusted.adjust(0, 0, GridPort::width(), 0);
                        break;
                }
                adjustedEdge[edge] = true;
            }
        }
        return adjusted;
    }

    std::map<Component*, int> connectedComponents() const { return m_component.connectedComponents(); }

    // Decorated versions of QRect functions
    // QRect::width, QRect::height adds +1 to their return values for "historical" reasons. This is undesirable for this
    // implementation, so overwrite this.
    inline int width() const { return QRect::width() - 1; }
    inline int height() const { return QRect::height() - 1; }

    // GridComponents should always be multiple-of-2 aligned for positioning algorithms to be able to halve
    // gridcomponent dimensions without getting a fraction as a return value.
    inline void moveTo(const QPoint& p) {
        Q_ASSERT(p.x() % 2 == 0 && p.y() % 2 == 0);
        QRect::moveTo(p);
        emitModified();
    }

    void setPortPosition(GridPort*& port, Edge edge, unsigned int offset) {
        switch (edge) {
            case Edge::Left:
            case Edge::Right:
                Q_ASSERT(offset <= static_cast<unsigned int>(height()));
                break;
            case Edge::Top:
            case Edge::Bottom:
                Q_ASSERT(offset <= static_cast<unsigned int>(width()));
                break;
        }
        port->setPosition(edge, offset);
    }

    Component& component() { return m_component; }
    const std::vector<GridComponent*> subcomponents() { return m_subcomponents; }

    /// Find grid position of member port @p port within this gridcomponent
    QPoint portPosition(GridPort* port) const {
        // Ensure that $port is a port of this gridcomponent
        Q_ASSERT(std::find_if(m_gridPorts.begin(), m_gridPorts.end(), [port](const auto& memberPort) {
                     return memberPort.get() == port;
                 }) != m_gridPorts.end());
        auto pos = port->getPosition();
        switch (pos.first) {
            case Edge::Left:
                return topLeft() + QPoint(0, static_cast<int>(pos.second));
            case Edge::Top:
                return topLeft() + QPoint(static_cast<int>(pos.second), 0);
            case Edge::Bottom:
                return bottomLeft() + QPoint(static_cast<int>(pos.second), 0);
            case Edge::Right:
                return topRight() + QPoint(0, static_cast<int>(pos.second));
        }
    }

    /* Signal emitted whenever the component has been modified, such as:
     * - Size changed
     * - Position changed
     * - Port position(s) changed
     */
    Gallant::Signal0<> modified;

private:
    void emitModified() { modified.Emit(); }

    std::vector<std::unique_ptr<GridPort>> m_gridPorts;
    std::vector<GridComponent*> m_subcomponents;

    Component& m_component;
    const QRect m_minimumRect;
};

}  // namespace eda
}  // namespace vsrtl

#endif  // GRIDCOMPONENT_H
