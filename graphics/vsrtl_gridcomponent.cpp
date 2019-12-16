#include "vsrtl_gridcomponent.h"

#include "vsrtl_graphics_util.h"

#include <math.h>

namespace vsrtl {

GridComponent::GridComponent(SimComponent& c, GridComponent* parent) : GraphicsBase(parent), m_component(c) {
    m_portPosMap = std::make_unique<PortPosMap>(c);

    updateMinimumGridRect();
    updateSubcomponentBoundingRect();
    m_currentContractedRect = m_minimumGridRect;
    m_currentExpandedRect = m_currentSubcomponentBoundingRect;

    // Initially, spread ports evenly over the component edges
    spreadPorts();
}

void GridComponent::setExpanded(bool state) {
    m_expanded = true;
    emit(expansionStateChanged(state));
}

bool GridComponent::adjust(int dx, int dy) {
    const auto& minRect = getCurrentMinRect();
    auto newRect = getCurrentComponentRect();
    newRect.adjust(0, 0, dx, dy);

    bool isMinRect = snapRectToRect(minRect, newRect);

    if (isMinRect)
        return false;

    if (!parentContainsRect(newRect.translated(m_relPos.get())))
        return false;

    updateCurrentComponentRect(dx, dy);
    return true;
}

bool GridComponent::move(CPoint<CSys::Parent> pos) {
    auto translatedRectInParentCS = getCurrentComponentRect().translated(pos.get());
    if (!parentContainsRect(translatedRectInParentCS))
        return false;

    m_relPos = pos;
    return true;
}

bool GridComponent::parentContainsRect(const QRect& r) const {
    if (parentItem() == nullptr)
        return true;

    auto* gc_parent = dynamic_cast<GridComponent*>(parent());
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

QRect& GridComponent::getCurrentComponentRect() {
    return m_expanded ? m_currentExpandedRect : m_currentContractedRect;
}

const QRect& GridComponent::getCurrentMinRect() {
    return m_expanded ? m_currentSubcomponentBoundingRect : m_minimumGridRect;
}

void GridComponent::updateSubcomponentBoundingRect() {
    auto tmp = collect<QRect>(getGridSubcomponents(), &GridComponent::getCurrentComponentRect);
    m_currentSubcomponentBoundingRect = boundingRectOfRects<QRect>(tmp);
}

void GridComponent::updateMinimumGridRect() {
    QRect shapeMinRect = ShapeRegister::getComponentMinGridRect(m_component.getGraphicsID());
    const auto n_inPorts = m_component.getPorts<SimPort::Direction::in>().size();
    const auto n_outPorts = m_component.getPorts<SimPort::Direction::out>().size();
    const auto largestPortSize = n_inPorts > n_outPorts ? n_inPorts : n_outPorts;
    const auto heightToAdd = (largestPortSize + 2) - shapeMinRect.height();
    if (heightToAdd > 0) {
        shapeMinRect.adjust(0, 0, 0, heightToAdd);
    }
    m_minimumGridRect = shapeMinRect;
}

void GridComponent::updateCurrentComponentRect(int dx, int dy) {
    getCurrentComponentRect().adjust(0, 0, dx, dy);
    spreadPorts();
}

void GridComponent::spreadPortsOnSide(const Side& side) {
    auto& biMap = m_portPosMap->dirToMap(side);
    const auto n_ports = biMap.count();
    if (n_ports > 0) {
        unsigned i = 0;
        const unsigned in_seg_y = std::floor(getCurrentComponentRect().height() / n_ports);
        assert(in_seg_y != 0);
        for (const auto& p : biMap.portToId) {
            const unsigned gridIndex = i * in_seg_y;
            bool portMoved = m_portPosMap->movePort(p.first, PortPos{side, gridIndex});
            if (portMoved) {
                emit portPosChanged(p.first);
            }
            i++;
        }
    }
}

void GridComponent::spreadPorts() {
    spreadPortsOnSide(Side::Left);
    spreadPortsOnSide(Side::Right);
    spreadPortsOnSide(Side::Top);
    spreadPortsOnSide(Side::Bottom);
}

}  // namespace vsrtl
