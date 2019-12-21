#include "vsrtl_gridcomponent.h"

#include "vsrtl_graphics_util.h"

#include <math.h>

namespace vsrtl {

GridComponent::GridComponent(SimComponent& c, GridComponent* parent) : GraphicsBase(parent), m_component(c) {
    m_border = std::make_unique<ComponentBorder>(c);

    updateMinimumGridRect();
    updateSubcomponentBoundingRect();
    m_currentContractedRect = m_minimumGridRect;
    m_currentExpandedRect = m_currentSubcomponentBoundingRect;
}

void GridComponent::setExpanded(bool state) {
    m_expanded = state;

    // This component just expanded - this might require the parent component to expand its current bounding rect
    auto* parent = dynamic_cast<GridComponent*>(parentItem());
    if (parent)
        parent->updateSubcomponentBoundingRect();

    emit gridRectChanged();
}

bool GridComponent::adjust(const QPoint& p) {
    const auto& minRect = getCurrentMinRect();
    auto newRect = getCurrentComponentRect();
    newRect.adjust(0, 0, p.x(), p.y());

    snapRectToRect(minRect, newRect);

    if (!parentContainsRect(newRect.translated(m_relPos.get())))
        return false;

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
    if (updateSubcomponentBoundingRect())
        emit gridRectChanged();
}

bool GridComponent::move(CPoint<CSys::Parent> pos) {
    auto translatedRectInParentCS = getCurrentComponentRect().translated(pos.get());
    if (!parentContainsRect(translatedRectInParentCS))
        return false;

    m_relPos = pos;
    return true;
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
    const auto rects = collect<QRect>(getGridSubcomponents(), &GridComponent::getCurrentComponentRect);
    const auto br = boundingRectOfRects<QRect>(rects);
    if (br != m_currentSubcomponentBoundingRect) {
        m_currentExpandedRect = br;
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
