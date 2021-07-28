#include "vsrtl_gridcomponent.h"

#include "eda/vsrtl_placeroute.h"
#include "vsrtl_graphics_defines.h"
#include "vsrtl_graphics_util.h"

#include <math.h>

namespace vsrtl {

GridComponent::GridComponent(SimComponent* c, GridComponent* parent) : GraphicsBaseItem(parent), m_component(c) {
    m_border = std::make_unique<ComponentBorder>(c);

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

    if (!parentIsPlacing() && !isSerializing()) {
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
    if (!isSerializing()) {
        updateSubcomponentBoundingRect();
    }
}

bool GridComponent::hasSubcomponents() const {
    return m_component->hasSubcomponents();
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
    m_prresult = eda::PlaceRoute::placeAndRoute(getGridSubcomponents());

    // Move components to their final positions
    for (const auto& p : m_prresult.placement.components) {
        p->gridComponent->move(p->pos);
    }
    // Let parent graphical component create routes
    applyRouteRes();

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
    const auto children = childItems();
    for (const auto& subc : qAsConst(children)) {
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

QRect GridComponent::getCurrentMinRect() const {
    return m_expanded ? m_currentSubcomponentBoundingRect : getContractedMinimumGridRect();
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
    const auto preferredRect = ShapeRegister::getTypePreferredRect(m_component->getGraphicsType());

    auto initialRect = getContractedMinimumGridRect();
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

QRect GridComponent::getContractedMinimumGridRect() const {
    // The contracted minimum grid rect is defined as a 1x1 rectangle, with each side being elongated by the number of
    // ports on that side
    QRect shapeMinRect = QRect(0, 0, 1, 1);

    const unsigned maxVerticalPorts =
        m_border->sideToMap(Direction::West).count() > m_border->sideToMap(Direction::East).count()
            ? m_border->sideToMap(Direction::West).count()
            : m_border->sideToMap(Direction::East).count();

    const unsigned maxHorizontalPorts =
        m_border->sideToMap(Direction::North).count() > m_border->sideToMap(Direction::South).count()
            ? m_border->sideToMap(Direction::North).count()
            : m_border->sideToMap(Direction::South).count();

    shapeMinRect.adjust(0, 0, maxHorizontalPorts, maxVerticalPorts);

    return shapeMinRect;
}

void GridComponent::gridRotate(const RotationDirection& dir) {
    m_lastComponentRect = getCurrentComponentRect();
    m_gridRotation += (dir == RotationDirection::RightHand ? 90 : -90);
    getCurrentComponentRectRef().setHeight(m_lastComponentRect.width());
    getCurrentComponentRectRef().setWidth(m_lastComponentRect.height());

    rotatePorts(dir);
    emit gridRectChanged();
}

void GridComponent::updateCurrentComponentRect(int dx, int dy) {
    m_lastComponentRect = getCurrentComponentRect();
    getCurrentComponentRectRef().adjust(0, 0, dx, dy);

    spreadPortsOrdered();

    // Port spreading only emits port positioning update signals when a port changes position logically on a given side.
    // If dx !^ dy, the component is adjusted in only a single direction. As such, ports on the side in the given change
    // direction will not move logically, but must be adjusted in terms of where they are drawn.
    if ((dx == 0) ^ (dy == 0)) {
        auto axisMovedPorts = dx == 0 ? m_border->sideToMap(Direction::South) : m_border->sideToMap(Direction::East);
        for (const auto& p : axisMovedPorts.portToId) {
            emit portPosChanged(p.first);
        }
    }
    emit gridRectChanged();
}

PortPos GridComponent::getPortPos(const SimPort* port) const {
    return m_border->getPortPos(port);
}

QPoint GridComponent::getPortGridPos(const SimPort* port) const {
    auto portIdx = m_border->getPortPos(port);
    auto rect = getCurrentComponentRect();
    QPoint portPos = rect.topLeft();
    switch (portIdx.side) {
        case Direction::North:
            portPos.rx() += portIdx.index;
            break;
        case Direction::West:
            portPos.ry() += portIdx.index;
            break;
        case Direction::East:
            portPos.rx() += rect.width();
            portPos.ry() += portIdx.index;
            break;
        case Direction::South:
            portPos.ry() += rect.height();
            portPos.rx() += portIdx.index;
            break;
    }
    return portPos;
}

std::vector<unsigned> GridComponent::getFreePortPositions(Direction s) {
    std::vector<unsigned> freePos;
    const auto& usedIndexes = m_border->sideToMap(s).idToPort;
    for (int i = 0; i < getCurrentComponentRect().height(); i++) {
        if (usedIndexes.count(i) == 0) {
            freePos.push_back(i);
        }
    }
    return freePos;
}

bool GridComponent::adjustPort(SimPort* port, QPoint newPos) {
    const auto ccr = getCurrentComponentRect();

    const bool inHz = -1 <= newPos.x() && newPos.x() <= ccr.width();
    const bool inVt = -1 <= newPos.y() && newPos.y() <= ccr.height();
    const bool inTopLeftCorner = newPos.x() == -1 && newPos.y() == -1;
    const bool inTopRightCorner = newPos.x() == ccr.width() && newPos.y() == -1;
    const bool inBotLeftCorner = newPos.x() == -1 && newPos.y() == ccr.height();
    const bool inBotRightCorner = newPos.x() == ccr.width() && newPos.y() == ccr.height();
    const bool valid = inHz && inVt && !(inTopLeftCorner || inTopRightCorner || inBotLeftCorner || inBotRightCorner);

    if (!valid) {
        // Out of bounds
        return false;
    }

    if ((newPos.x() > -1 && newPos.x() < ccr.width()) && (newPos.y() > -1 && newPos.y() < ccr.height())) {
        // newPos is inside this component
        return false;
    }

    PortPos newPortPos;

    // Snap port position to the border of the component
    newPos.rx() = newPos.x() < 0 ? -1 : newPos.x();
    newPos.rx() = newPos.x() >= ccr.width() ? ccr.width() : newPos.x();

    newPos.ry() = newPos.y() < 0 ? -1 : newPos.y();
    newPos.ry() = newPos.y() >= ccr.height() ? ccr.height() : newPos.y();

    if (newPos.x() == -1) {
        newPortPos.side = Direction::West;
        newPortPos.index = newPos.y();
    } else if (newPos.x() == ccr.width()) {
        newPortPos.side = Direction::East;
        newPortPos.index = newPos.y();
    } else if (newPos.y() == -1) {
        newPortPos.side = Direction::North;
        newPortPos.index = newPos.x();
    } else if (newPos.y() == ccr.height()) {
        newPortPos.side = Direction::South;
        newPortPos.index = newPos.x();
    } else {
        Q_ASSERT(false && "Error in snapping or out-of-bounds handling");
    }

    if (newPortPos == m_border->getPortPos(port)) {
        // No change, same index
        return false;
    }

    const auto movedPorts = m_border->movePort(port, newPortPos);
    for (const auto& p : movedPorts) {
        emit portPosChanged(p);
    }
    return movedPorts.size() > 0;
}

void GridComponent::rotatePorts(const RotationDirection& dir) {
    std::vector<std::pair<Direction, ComponentBorder::PortIdBiMap>> oldPorts;
    for (const auto& side : {Direction::North, Direction::East, Direction::West, Direction::South}) {
        oldPorts.push_back({side, m_border->sideToMap(side)});
    }

    for (const auto& sidePorts : oldPorts) {
        for (const auto& port : sidePorts.second.portToId) {
            Direction newSide = Direction();
            // clang-format off
            switch(sidePorts.first) {
                case Direction::North:     newSide = dir == RotationDirection::RightHand ? Direction::East : Direction::West; break;
                case Direction::West:    newSide = dir == RotationDirection::RightHand ? Direction::North : Direction::South; break;
                case Direction::East:   newSide = dir == RotationDirection::RightHand ? Direction::South : Direction::North; break;
                case Direction::South:  newSide = dir == RotationDirection::RightHand ? Direction::West : Direction::East; break;
            }
            // clang-format on

            m_border->movePort(port.first, PortPos{newSide, port.second});
            emit portPosChanged(port.first);
        }
    }
}

void GridComponent::spreadPortsOnSide(const Direction& side) {
    auto biMapCopy = m_border->sideToMap(side);
    const auto n_ports = biMapCopy.count();
    if (n_ports > 0) {
        int i = 0;
        const double h = getCurrentComponentRect().height();
        const double diff = h / n_ports;
        for (const auto& portId : biMapCopy.portToId) {
            const int gridIndex = static_cast<int>(std::floor((i * diff + diff / 2)));
            const auto* port = portId.first;  // Store port pointer here; p reference may change during port moving
            const auto movedPorts = m_border->movePort(port, PortPos{side, gridIndex});
            for (const auto& p : movedPorts) {
                emit portPosChanged(p);
            }
            i++;
        }
    }
}

void GridComponent::spreadPortsOrdered() {
    for (const auto& side : {Direction::North, Direction::East, Direction::West, Direction::South}) {
        auto biMapCopy = m_border->sideToMap(side);
        const auto n_ports = biMapCopy.count();
        if (n_ports > 0) {
            int i = 0;
            const double diff =
                ((side == Direction::West || side == Direction::East) ? getCurrentComponentRect().height()
                                                                      : getCurrentComponentRect().width()) /
                n_ports;
            for (const auto& idp : biMapCopy.idToPort) {
                const int gridIndex = static_cast<int>(std::ceil((i * diff + diff / 2)));
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
    spreadPortsOnSide(Direction::North);
    spreadPortsOnSide(Direction::East);
    spreadPortsOnSide(Direction::West);
    spreadPortsOnSide(Direction::South);
}

}  // namespace vsrtl
