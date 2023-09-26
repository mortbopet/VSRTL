#include "vsrtl_gridcomponent.h"

#include "vsrtl_graphics_defines.h"
#include "vsrtl_graphics_util.h"
#include "vsrtl_placeroute.h"

#include <math.h>

namespace vsrtl {

GridComponent::GridComponent(SimComponent *c, GridComponent *parent)
    : GraphicsBaseItem(parent), m_component(c),
      m_border(std::make_unique<ComponentBorder>(c)) {
  setInitialRect();
  m_currentExpandedRect = m_currentSubcomponentBoundingRect;
}

void GridComponent::setExpanded(bool state) {
  if (!hasSubcomponents())
    return;

  m_lastComponentRect = getCurrentComponentRect();
  m_expanded = state;
  spreadPortsOrdered();

  // This component just modified its geometry - this might require the parent
  // component to expand its current bounding rect
  auto *parent = dynamic_cast<GridComponent *>(parentItem());
  if (parent && hasSubcomponents())
    parent->childGeometryChanged();

  emit gridRectChanged();
}

bool GridComponent::adjust(const QPoint &p) {
  if (p == QPoint(0, 0))
    return false;

  const auto &minRect = getCurrentMinRect();
  auto newRect = getCurrentComponentRect();
  newRect.adjust(0, 0, p.x(), p.y());

  if (!parentIsPlacing() && !isSerializing()) {
    // Parent is not placing components, snap subcomponents inside rect inside
    // parent
    snapRectToInnerRect(minRect, newRect);
    auto *parent = dynamic_cast<GridComponent *>(parentItem());
    if (parent) {
      // Snap new rect to stay within parent
      newRect.translate(m_relPos);
      snapRectToOuterRect(parent->getCurrentComponentRect(), newRect);
      newRect.translate(-m_relPos);
    }
  }

  // Rect is now snapped, calculate difference between current and new rect
  const QPoint diff =
      newRect.bottomRight() - getCurrentComponentRect().bottomRight();

  updateCurrentComponentRect(diff.x(), diff.y());

  auto *parent = dynamic_cast<GridComponent *>(parentItem());
  if (parent && (p.x() > 0 || p.y() > 0)) {
    parent->childGeometryChanged();
  }

  return true;
}

bool GridComponent::adjust(const QRect &newRect) {
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

bool GridComponent::move(const QPoint &pos) {
  if (parentIsPlacing()) {
    // Parent is placing components, do not try to snap inside parent
    m_relPos = pos;
    emit gridPosChanged(m_relPos);
    return true;
  }

  // Restrict positioning to inside parent rect
  QPoint newPos = pos;
  auto *parent = dynamic_cast<GridComponent *>(parentItem());
  if (parent) {
    newPos.setX(qMin(parent->getCurrentComponentRect().right() + 1 -
                         getCurrentComponentRect().width(),
                     qMax(newPos.x(), 0)));
    newPos.setY(qMin(parent->getCurrentComponentRect().bottom() + 1 -
                         getCurrentComponentRect().height(),
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
  const auto &placements =
      PlaceRoute::get()->placeAndRoute(getGridSubcomponents());
  for (const auto &p : placements) {
    p.first->move(p.second);
  }
  m_isPlacing = false;
  updateSubcomponentBoundingRect();
}

bool GridComponent::parentIsPlacing() const {
  auto *p = dynamic_cast<GridComponent *>(parentItem());
  if (p)
    return p->m_isPlacing;
  return false;
}

bool GridComponent::parentContainsRect(const QRect &r) const {
  auto *gc_parent = dynamic_cast<GridComponent *>(parentItem());
  if (gc_parent == nullptr)
    return true;

  return gc_parent->getCurrentComponentRect().contains(r);
}

std::vector<GridComponent *> GridComponent::getGridSubcomponents() const {
  std::vector<GridComponent *> c;
  const auto children = childItems();
  for (const auto &subc : std::as_const(children)) {
    auto *ptr = dynamic_cast<GridComponent *>(subc);
    if (ptr)
      c.push_back(ptr);
  }
  return c;
}

QRect &GridComponent::getCurrentComponentRectRef() {
  return m_expanded ? m_currentExpandedRect : m_currentContractedRect;
}

const QRect &GridComponent::getCurrentComponentRect() const {
  return m_expanded ? m_currentExpandedRect : m_currentContractedRect;
}

QRect GridComponent::getCurrentMinRect() const {
  return m_expanded ? m_currentSubcomponentBoundingRect
                    : getContractedMinimumGridRect();
}

bool GridComponent::updateSubcomponentBoundingRect() {
  if (hasSubcomponents()) {
    std::vector<QRect> rects;
    for (const auto &c : getGridSubcomponents()) {
      rects.push_back(c->getCurrentComponentRect().translated(c->getGridPos()));
    }
    const auto br = boundingRectOfRects<QRect>(rects);
    m_currentSubcomponentBoundingRect = br;
    // Update current expanded rect if it does not contain the subcomponent
    // bounding rect
    if (!m_currentExpandedRect.contains(m_currentSubcomponentBoundingRect)) {
      m_currentExpandedRect = br;
      m_currentExpandedRect.setTopLeft({0, 0});
      m_currentExpandedRect.adjust(0, 0, SUBCOMPONENT_INDENT,
                                   SUBCOMPONENT_INDENT);
      emit gridRectChanged();
    }
    return true;
  }
  return false;
}

void GridComponent::setInitialRect() {
  const auto preferredRect =
      ShapeRegister::getTypePreferredRect(m_component->getGraphicsType());

  auto initialRect = getContractedMinimumGridRect();
  if (preferredRect == QRect()) {
    // No preferred size, adjust width heuristically based on height of
    // component
    const auto widthToAdd =
        static_cast<int>(std::floor(std::log2(initialRect.height())));
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
  // The contracted minimum grid rect is defined as a 1x1 rectangle, with each
  // side being elongated by the number of ports on that side
  QRect shapeMinRect = QRect(0, 0, 1, 1);

  const unsigned maxVerticalPorts =
      m_border->sideToMap(Side::Left).count() >
              m_border->sideToMap(Side::Right).count()
          ? m_border->sideToMap(Side::Left).count()
          : m_border->sideToMap(Side::Right).count();

  const unsigned maxHorizontalPorts =
      m_border->sideToMap(Side::Top).count() >
              m_border->sideToMap(Side::Bottom).count()
          ? m_border->sideToMap(Side::Top).count()
          : m_border->sideToMap(Side::Bottom).count();

  shapeMinRect.adjust(0, 0, maxHorizontalPorts, maxVerticalPorts);

  return shapeMinRect;
}

void GridComponent::gridRotate(const RotationDirection &dir) {
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

  // Port spreading only emits port positioning update signals when a port
  // changes position logically on a given side. If dx !^ dy, the component is
  // adjusted in only a single direction. As such, ports on the side in the
  // given change direction will not move logically, but must be adjusted in
  // terms of where they are drawn.
  if ((dx == 0) ^ (dy == 0)) {
    auto axisMovedPorts = dx == 0 ? m_border->sideToMap(Side::Bottom)
                                  : m_border->sideToMap(Side::Right);
    for (const auto &p : axisMovedPorts.portToId) {
      emit portPosChanged(p.first);
    }
  }
  emit gridRectChanged();
}

PortPos GridComponent::getPortPos(const SimPort *port) const {
  return m_border->getPortPos(port);
}

std::vector<unsigned> GridComponent::getFreePortPositions(Side s) {
  std::vector<unsigned> freePos;
  const auto &usedIndexes = m_border->sideToMap(s).idToPort;
  for (int i = 0; i < getCurrentComponentRect().height(); i++) {
    if (usedIndexes.count(i) == 0) {
      freePos.push_back(i);
    }
  }
  return freePos;
}

bool GridComponent::adjustPort(SimPort *port, QPoint newPos) {
  const auto ccr = getCurrentComponentRect();
  if ((newPos.x() <= 0 && newPos.y() <= 0) ||
      (newPos.x() >= ccr.width() && newPos.y() >= ccr.height()) ||
      (newPos.x() <= 0 && newPos.y() >= ccr.height()) ||
      (newPos.x() >= ccr.width() && newPos.y() <= 0)) {
    // Out of bounds
    return false;
  }

  if ((newPos.x() > 0 && newPos.x() < ccr.width()) &&
      (newPos.y() > 0 && newPos.y() < ccr.height())) {
    // newPos is inside this component
    return false;
  }

  PortPos newPortPos;

  // Snap port position to the border of the component
  newPos.rx() = newPos.x() < 0 ? 0 : newPos.x();
  newPos.rx() = newPos.x() > ccr.width() ? ccr.width() : newPos.x();

  newPos.ry() = newPos.y() < 0 ? 0 : newPos.y();
  newPos.ry() = newPos.y() > ccr.height() ? ccr.height() : newPos.y();

  if (newPos.x() == 0) {
    newPortPos.side = Side::Left;
    newPortPos.index = newPos.y();
  } else if (newPos.x() == ccr.width()) {
    newPortPos.side = Side::Right;
    newPortPos.index = newPos.y();
  } else if (newPos.y() == 0) {
    newPortPos.side = Side::Top;
    newPortPos.index = newPos.x();
  } else if (newPos.y() == ccr.height()) {
    newPortPos.side = Side::Bottom;
    newPortPos.index = newPos.x();
  } else {
    Q_ASSERT(false && "Error in snapping or out-of-bounds handling");
  }

  if (newPortPos == m_border->getPortPos(port)) {
    // No change, same index
    return false;
  }

  const auto movedPorts = m_border->movePort(port, newPortPos);
  for (const auto &p : movedPorts) {
    emit portPosChanged(p);
  }
  return movedPorts.size() > 0;
}

void GridComponent::rotatePorts(const RotationDirection &dir) {
  std::vector<std::pair<Side, ComponentBorder::PortIdBiMap>> oldPorts;
  for (const auto &side : {Side::Left, Side::Right, Side::Top, Side::Bottom}) {
    oldPorts.push_back({side, m_border->sideToMap(side)});
  }

  for (const auto &sidePorts : oldPorts) {
    for (const auto &port : sidePorts.second.portToId) {
      Side newSide = Side();
      // clang-format off
            switch(sidePorts.first) {
                case Side::Top:     newSide = dir == RotationDirection::RightHand ? Side::Right : Side::Left; break;
                case Side::Left:    newSide = dir == RotationDirection::RightHand ? Side::Top : Side::Bottom; break;
                case Side::Right:   newSide = dir == RotationDirection::RightHand ? Side::Bottom : Side::Top; break;
                case Side::Bottom:  newSide = dir == RotationDirection::RightHand ? Side::Left : Side::Right; break;
            }
      // clang-format on

      m_border->movePort(port.first, PortPos{newSide, port.second});
      emit portPosChanged(port.first);
    }
  }
}

void GridComponent::spreadPortsOnSide(const Side &side) {
  auto biMapCopy = m_border->sideToMap(side);
  const auto n_ports = biMapCopy.count();
  if (n_ports > 0) {
    int i = 0;
    auto h = getCurrentComponentRect().height();
    const double diff = h / n_ports;
    for (const auto &portId : biMapCopy.portToId) {
      const int gridIndex = static_cast<int>(std::ceil((i * diff + diff / 2)));
      const auto *port = portId.first; // Store port pointer here; p reference
                                       // may change during port moving
      const auto movedPorts =
          m_border->movePort(port, PortPos{side, gridIndex});
      for (const auto &p : movedPorts) {
        emit portPosChanged(p);
      }
      i++;
    }
  }
}

void GridComponent::spreadPortsOrdered() {
  for (const auto &side : {Side::Left, Side::Right, Side::Top, Side::Bottom}) {
    auto biMapCopy = m_border->sideToMap(side);
    const auto n_ports = biMapCopy.count();
    if (n_ports > 0) {
      int i = 0;
      const double diff = ((side == Side::Left || side == Side::Right)
                               ? getCurrentComponentRect().height()
                               : getCurrentComponentRect().width()) /
                          n_ports;
      for (const auto &idp : biMapCopy.idToPort) {
        const int gridIndex =
            static_cast<int>(std::ceil((i * diff + diff / 2)));
        const auto *port = idp.second; // Store port pointer here; p reference
                                       // may change during port moving
        const auto movedPorts =
            m_border->movePort(port, PortPos{side, gridIndex});
        for (const auto &p : movedPorts) {
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
  spreadPortsOnSide(Side::Top);
  spreadPortsOnSide(Side::Bottom);
}

} // namespace vsrtl
