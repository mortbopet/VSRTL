#include "vsrtl_gridcomponent.h"

#include "vsrtl_graphics_defines.h"
#include "vsrtl_graphics_util.h"
#include "vsrtl_placeroute.h"

#include <math.h>

namespace vsrtl {

GridComponent::GridComponent(SimComponent& c, GridComponent* parent) : GraphicsBase(parent), m_component(c) {
    m_border = std::make_unique<ComponentBorder>(c);

    updateMinimumGridRect();
    m_currentContractedRect = m_minimumGridRect;
    m_currentExpandedRect = m_currentSubcomponentBoundingRect;
}

void GridComponent::setExpanded(bool state) {
    if (!m_component.hasSubcomponents())
        return;

    m_expanded = state;

    // This component just expanded - this might require the parent component to expand its current bounding rect
    auto* parent = dynamic_cast<GridComponent*>(parentItem());
    if (state && parent && m_component.hasSubcomponents())
        parent->childExpanded();

    emit gridRectChanged();
}

bool GridComponent::adjust(const QPoint& p) {
    const auto& minRect = getCurrentMinRect();
    auto newRect = getCurrentComponentRect();
    newRect.adjust(0, 0, p.x(), p.y());

    snapRectToInnerRect(minRect, newRect);
    auto* parent = dynamic_cast<GridComponent*>(parentItem());
    if (parent) {
        // Snap new rect to stay within parent
        newRect.translate(m_relPos.get());
        snapRectToOuterRect(parent->getCurrentComponentRect(), newRect);
        newRect.translate(-m_relPos.get());
    }

    // Rect is now snapped, calculate difference between current and new rect
    const QPoint diff = newRect.bottomRight() - getCurrentComponentRect().bottomRight();

    updateCurrentComponentRect(diff.x(), diff.y());
    return true;
}

bool GridComponent::adjust(const QRect& newRect) {
    auto r1 = getCurrentComponentRect();
    auto r2 = newRect;
    r2.setTopLeft({0, 0});
    r1.setTopLeft({0, 0});
    const QPoint diff = r2.bottomRight() - r1.bottomRight();
    return adjust(diff);
}

void GridComponent::childExpanded() {
    Q_ASSERT(m_expanded && "A child expanded while this component was collapsed");
    updateSubcomponentBoundingRect();
}

bool GridComponent::move(CPoint<CSys::Parent> pos) {
    if (parentIsPlacing()) {
        // Parent is placing components, do not try to snap inside parent
        m_relPos = pos;
        emit gridPosChanged(m_relPos.get());
        return true;
    }

    // Restrict positioning to inside parent rect
    const auto* parent = dynamic_cast<GridComponent*>(parentItem());
    QPoint newPos = pos.get();
    if (parent) {
        newPos.setX(qMin(parent->getCurrentComponentRect().right() + 1 - getCurrentComponentRect().width(),
                         qMax(newPos.x(), 0)));
        newPos.setY(qMin(parent->getCurrentComponentRect().bottom() + 1 - getCurrentComponentRect().height(),
                         qMax(newPos.y(), 0)));
    }

    auto translatedRectInParentCS = getCurrentComponentRect().translated(newPos);
    if (!parentIsPlacing() && !parentContainsRect(translatedRectInParentCS))
        return false;

    m_relPos = newPos;
    if (parentIsPlacing())
        emit gridPosChanged(m_relPos.get());

    return true;
}

void GridComponent::placeAndRouteSubcomponents() {
    m_isPlacing = true;
    const auto& placements = PlaceRoute::get()->placeAndRoute(getGridSubcomponents());
    for (const auto& p : placements) {
        p.first->move(p.second);
    }
    m_isPlacing = false;
    updateSubcomponentBoundingRect();
}

bool GridComponent::parentIsPlacing() const {
    auto* p = dynamic_cast<GridComponent*>(parentItem());
    if (p)
        return p->m_isPlacing;
    return false;
}

bool GridComponent::parentContainsRect(const QRect& r) const {
    auto* gc_parent = dynamic_cast<GridComponent*>(parentItem());
    if (gc_parent == nullptr)
        return true;

    return gc_parent->getCurrentComponentRect().contains(r);
}

CPoint<CSys::Parent> GridComponent::localToParentCoords(CPoint<CSys::Local> p) {
    return CPoint<CSys::Parent>(p.get() + m_relPos.get());
}

CPoint<CSys::Global> GridComponent::parentToGlobalCoords(CPoint<CSys::Parent> p) {
    auto* gc_parent = dynamic_cast<GridComponent*>(parent());
    if (gc_parent) {
        return localToGlobalCoords(CPoint<CSys::Local>(p.get()));
    } else {
        // This is the top level component (no parent) which represents global coordinates
        return CPoint<CSys::Global>(p.get());
    }
}

CPoint<CSys::Global> GridComponent::localToGlobalCoords(CPoint<CSys::Local> p) {
    return parentToGlobalCoords(localToParentCoords(p));
}

std::vector<GridComponent*> GridComponent::getGridSubcomponents() const {
    std::vector<GridComponent*> c;
    for (const auto& subc : childItems()) {
        auto* ptr = dynamic_cast<GridComponent*>(subc);
        if (ptr)
            c.push_back(ptr);
    }
    return c;
}

QRect& GridComponent::getCurrentComponentRectRef() {
    return m_expanded ? m_currentExpandedRect : m_currentContractedRect;
}

const QRect& GridComponent::getCurrentComponentRect() const {
    return m_expanded ? m_currentExpandedRect : m_currentContractedRect;
}

const QRect& GridComponent::getCurrentMinRect() const {
    return m_expanded ? m_currentSubcomponentBoundingRect : m_minimumGridRect;
}

bool GridComponent::updateSubcomponentBoundingRect() {
    if (m_component.hasSubcomponents()) {
        std::vector<QRect> rects;
        for (const auto& c : getGridSubcomponents()) {
            rects.push_back(c->getCurrentComponentRect().translated(c->getGridPos()));
        }
        const auto br = boundingRectOfRects<QRect>(rects);
        m_currentSubcomponentBoundingRect = br;
        // Update current expanded rect if it does not contain the subcomponent bounding rect
        if (!m_currentExpandedRect.contains(m_currentSubcomponentBoundingRect)) {
            m_currentExpandedRect = br;
            m_currentExpandedRect.setTopLeft({0, 0});
            m_currentExpandedRect.adjust(0, 0, SUBCOMPONENT_INDENT, SUBCOMPONENT_INDENT);
            emit gridRectChanged();
        }
        return true;
    }
    return false;
}

bool GridComponent::updateMinimumGridRect() {
    QRect shapeMinRect = ShapeRegister::getComponentMinGridRect(m_component.getGraphicsID());
    const auto n_inPorts = m_component.getPorts<SimPort::Direction::in>().size();
    const auto n_outPorts = m_component.getPorts<SimPort::Direction::out>().size();
    const auto largestPortSize = n_inPorts > n_outPorts ? n_inPorts : n_outPorts;
    const auto heightToAdd = (largestPortSize + 2) - shapeMinRect.height();
    if (heightToAdd > 0) {
        shapeMinRect.adjust(0, 0, 0, heightToAdd);
    }

    if (shapeMinRect != m_minimumGridRect) {
        m_minimumGridRect = shapeMinRect;
        return true;
    }
    return false;
}

bool GridComponent::updateCurrentComponentRect(int dx, int dy) {
    getCurrentComponentRectRef().adjust(0, 0, dx, dy);
    spreadPorts();

    // Port spreading only emits port positioning update signals when a port changes position logically on a given side.
    // If dx !^ dy, the component is adjusted in only a single direction. As such, ports on the side in the given change
    // direction will not move logically, but must be adjusted in terms of where they are drawn.
    if (dx == 0 ^ dy == 0) {
        auto axisMovedPorts = dx == 0 ? m_border->dirToMap(Side::Bottom) : m_border->dirToMap(Side::Right);
        for (const auto& p : axisMovedPorts.portToId) {
            emit portPosChanged(p.first);
        }
    }
    emit gridRectChanged();
}

PortPos GridComponent::getPortPos(const SimPort* port) const {
    return m_border->getPortPos(port);
}

void GridComponent::spreadPortsOnSide(const Side& side) {
    assert(side != Side::Top && side != Side::Bottom && "Not implemented");

    auto biMapCopy = m_border->dirToMap(side);
    const auto n_ports = biMapCopy.count();
    if (n_ports > 0) {
        int i = 0;
        auto h = getCurrentComponentRect().height();
        const double diff = getCurrentComponentRect().height() / n_ports;
        for (const auto& p : biMapCopy.portToId) {
            const int gridIndex = std::ceil((i * diff + diff / 2));
            const auto* port = p.first;  // Store port pointer here; p reference may change during port moving
            bool portMoved = m_border->movePort(port, PortPos{side, gridIndex});
            if (portMoved) {
                emit portPosChanged(port);
            }
            i++;
        }
    }
}

void GridComponent::spreadPorts() {
    spreadPortsOnSide(Side::Left);
    spreadPortsOnSide(Side::Right);
    // spreadPortsOnSide(Side::Top);
    // spreadPortsOnSide(Side::Bottom);
}

}  // namespace vsrtl
