#include "vsrtl_gridcomponent.h"

#include "vsrtl_graphics_defines.h"
#include "vsrtl_graphics_util.h"
#include "vsrtl_placeroute.h"

#include <math.h>

namespace vsrtl {

GridComponent::GridComponent(SimComponent& c, GridComponent* parent) : GraphicsBase(parent), m_component(c) {
    m_border = std::make_unique<ComponentBorder>(c);

    updateMinimumGridRect();
    setInitialRect();
    m_currentExpandedRect = m_currentSubcomponentBoundingRect;
}

void GridComponent::setExpanded(bool state) {
    if (!hasSubcomponents())
        return;

    m_lastComponentRect = getCurrentComponentRect();
    m_expanded = state;
    spreadPortsOrdered();

    // This component just modified its geometry - this might require the parent component to expand its current
    // bounding rect
    auto* parent = dynamic_cast<GridComponent*>(parentItem());
    if (parent && hasSubcomponents())
        parent->childGeometryChanged();

    emit gridRectChanged();
}

bool GridComponent::adjust(const QPoint& p) {
    if (p == QPoint(0, 0))
        return false;

    const auto& minRect = getCurrentMinRect();
    auto newRect = getCurrentComponentRect();
    newRect.adjust(0, 0, p.x(), p.y());

    if (!parentIsPlacing() && !m_serializing) {
        // Parent is not placing components, snap subcomponents inside rect inside parent
        snapRectToInnerRect(minRect, newRect);
        auto* parent = dynamic_cast<GridComponent*>(parentItem());
        if (parent) {
            // Snap new rect to stay within parent
            newRect.translate(m_relPos);
            snapRectToOuterRect(parent->getCurrentComponentRect(), newRect);
            newRect.translate(-m_relPos);
        }
    }

    // Rect is now snapped, calculate difference between current and new rect
    const QPoint diff = newRect.bottomRight() - getCurrentComponentRect().bottomRight();

    updateCurrentComponentRect(diff.x(), diff.y());

    auto* parent = dynamic_cast<GridComponent*>(parentItem());
    if (parent && (p.x() > 0 || p.y() > 0)) {
        parent->childGeometryChanged();
    }

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

void GridComponent::childGeometryChanged() {
    if (!m_serializing) {
        updateSubcomponentBoundingRect();
    }
}

bool GridComponent::hasSubcomponents() const {
    return m_component.hasSubcomponents();
}

bool GridComponent::move(const QPoint& pos) {
    if (parentIsPlacing()) {
        // Parent is placing components, do not try to snap inside parent
        m_relPos = pos;
        emit gridPosChanged(m_relPos);
        return true;
    }

    // Restrict positioning to inside parent rect
    auto* parent = dynamic_cast<GridComponent*>(parentItem());
    QPoint newPos = pos;
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
    if (parent) {
        parent->childGeometryChanged();
    }

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
    if (hasSubcomponents()) {
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

void GridComponent::setInitialRect() {
    const auto preferredRect = ShapeRegister::getComponentPreferredRect(m_component.getGraphicsID());

    auto initialRect = m_minimumGridRect;
    if (preferredRect == QRect()) {
        // No preferred size, adjust width heuristically based on height of component
        const auto widthToAdd = static_cast<int>(std::floor(std::log2(initialRect.height())));
        initialRect.adjust(0, 0, widthToAdd, 0);
    } else {
        // Attempt to adjust according to the preferred dimensions of the component
        auto heightToAdd = preferredRect.height() - initialRect.height();
        heightToAdd = heightToAdd < 0 ? 0 : heightToAdd;
        auto widthToAdd = preferredRect.width() - initialRect.width();
        widthToAdd = widthToAdd < 0 ? 0 : widthToAdd;
        initialRect.adjust(0, 0, widthToAdd, heightToAdd);
    }

    m_currentContractedRect = initialRect;
}

bool GridComponent::updateMinimumGridRect() {
    QRect shapeMinRect = QRect(0, 0, 1, 1);  //
    const auto n_inPorts = m_component.getPorts<SimPort::Direction::in>().size();
    const auto n_outPorts = m_component.getPorts<SimPort::Direction::out>().size();
    const auto largestPortSize = n_inPorts > n_outPorts ? n_inPorts : n_outPorts;
    const auto heightToAdd = largestPortSize + 1 - shapeMinRect.height();
    shapeMinRect.adjust(0, 0, 0, heightToAdd);

    if (shapeMinRect != m_minimumGridRect) {
        m_minimumGridRect = shapeMinRect;
        return true;
    }
    return false;
}

bool GridComponent::updateCurrentComponentRect(int dx, int dy) {
    m_lastComponentRect = getCurrentComponentRect();
    getCurrentComponentRectRef().adjust(0, 0, dx, dy);

    spreadPortsOrdered();

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

std::vector<unsigned> GridComponent::getFreePortPositions(Side s) {
    std::vector<unsigned> freePos;
    const auto& usedIndexes = m_border->dirToMap(s).idToPort;
    for (int i = 0; i < getCurrentComponentRect().height(); i++) {
        if (usedIndexes.count(i) == 0) {
            freePos.push_back(i);
        }
    }
    return freePos;
}

bool GridComponent::adjustPort(SimPort* port, unsigned newPos) {
    if ((newPos == 0) || (newPos >= getCurrentComponentRect().height()))
        return false;

    auto adjustedPos = m_border->getPortPos(port);
    if (adjustedPos.index == newPos) {
        // No change, same index
        return false;
    }
    adjustedPos.index = newPos;
    const auto movedPorts = m_border->movePort(port, adjustedPos);
    for (const auto& p : movedPorts) {
        emit portPosChanged(p);
    }
    return movedPorts.size() > 0;
}

void GridComponent::spreadPortsOnSide(const Side& side) {
    assert(side != Side::Top && side != Side::Bottom && "Not implemented");

    auto biMapCopy = m_border->dirToMap(side);
    const auto n_ports = biMapCopy.count();
    if (n_ports > 0) {
        int i = 0;
        auto h = getCurrentComponentRect().height();
        const double diff = h / n_ports;
        for (const auto& p : biMapCopy.portToId) {
            const int gridIndex = std::ceil((i * diff + diff / 2));
            const auto* port = p.first;  // Store port pointer here; p reference may change during port moving
            const auto movedPorts = m_border->movePort(port, PortPos{side, gridIndex});
            for (const auto& p : movedPorts) {
                emit portPosChanged(p);
            }
            i++;
        }
    }
}

void GridComponent::spreadPortsOrdered() {
    for (const auto& side : {Side::Left, Side::Right}) {
        auto biMapCopy = m_border->dirToMap(side);
        const auto n_ports = biMapCopy.count();
        if (n_ports > 0) {
            int i = 0;
            auto h = getCurrentComponentRect().height();
            const double diff = getCurrentComponentRect().height() / n_ports;
            for (const auto& idp : biMapCopy.idToPort) {
                const int gridIndex = std::ceil((i * diff + diff / 2));
                const auto* port = idp.second;  // Store port pointer here; p reference may change during port moving
                const auto movedPorts = m_border->movePort(port, PortPos{side, gridIndex});
                for (const auto& p : movedPorts) {
                    emit portPosChanged(p);
                }
                i++;
            }
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
