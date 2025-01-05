#include "vsrtl_scene.h"

#include <algorithm>
#include <iterator>

#include <QAction>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsSceneMouseEvent>
#include <QMenu>

#include "vsrtl_componentgraphic.h"
#include "vsrtl_portgraphic.h"
#include "vsrtl_wiregraphic.h"

namespace vsrtl {

template <typename T>
T *getSingleSelectedItem(const QGraphicsScene &scene) {
  const auto selectedItems = scene.selectedItems();
  if (selectedItems.size() != 1) {
    return nullptr;
  }
  return dynamic_cast<T *>(selectedItems.at(0));
}

VSRTLScene::VSRTLScene(QObject *parent) : QGraphicsScene(parent) {
  connect(this, &QGraphicsScene::selectionChanged, this,
          &VSRTLScene::handleSelectionChanged);

  m_darkmodeAction = new QAction("Darkmode", this);
  m_darkmodeAction->setCheckable(true);
  m_darkmodeAction->setChecked(m_darkmode);
  connect(m_darkmodeAction, &QAction::toggled, [=](bool checked) {
    m_darkmode = checked;

    // Background
    this->setBackgroundBrush(m_darkmode
                                 ? QBrush(QColorConstants::DarkGray.darker(300))
                                 : Qt::NoBrush);

    this->update();
  });
}

/**
 * @brief VSRTLScene::handleWirePointMove
 *  For supporting drag/drop of points simultaneously with allowing point
 * movement, we cannot use QDrag. Instead, the scene will manage whether a
 * single point is currently being dragged - if so, it will keep track of the
 * WirePoint's underneath the cursor, and notify them. From this,we are then
 * able to highlight the WirePoint's which are eligible for merging.
 */
void VSRTLScene::handleWirePointMove(QGraphicsSceneMouseEvent *event) {
  if (m_selectedPoint != nullptr && event->buttons() == Qt::LeftButton) {
    std::set<WirePoint *> pointsUnderCursor;
    const auto itemsAtPoint = items(event->scenePos());
    for (const auto &item : std::as_const(itemsAtPoint)) {
      if (auto *point = dynamic_cast<WirePoint *>(item)) {
        if (m_selectedPoint->canMergeWith(point)) {
          pointsUnderCursor.insert(point);
        }
      }
    }

    pointsUnderCursor.erase(m_selectedPoint);

    // notify any new points that they are now potential drop targets
    std::set<WirePoint *> diff;
    std::set_difference(pointsUnderCursor.begin(), pointsUnderCursor.end(),
                        m_currentDropTargets.begin(),
                        m_currentDropTargets.end(),
                        std::inserter(diff, diff.begin()));

    for (const auto &point : diff)
      point->pointDragEnter(m_selectedPoint);

    // Clear any old points which are no longer under the curser
    std::set<WirePoint *> oldPoints;
    std::set_difference(m_currentDropTargets.begin(),
                        m_currentDropTargets.end(), pointsUnderCursor.begin(),
                        pointsUnderCursor.end(),
                        std::inserter(oldPoints, oldPoints.begin()));

    for (const auto &oldPoint : oldPoints) {
      oldPoint->pointDragLeave(m_selectedPoint);
      m_currentDropTargets.erase(oldPoint);
    }

    // Add new points to the currently tracked drop targets
    m_currentDropTargets.insert(diff.begin(), diff.end());

  } else {
    for (const auto &oldTarget : m_currentDropTargets) {
      oldTarget->pointDragLeave(m_selectedPoint);
    }
    m_currentDropTargets.clear();
  }
}

void VSRTLScene::drawBackground(QPainter *painter, const QRectF &rect) {
  if (backgroundBrush() != Qt::NoBrush) {
    QGraphicsScene::drawBackground(painter, rect);
  }

  if (!m_showGrid)
    return;

  painter->save();
  painter->setPen(QPen(Qt::lightGray, 1));

  // Align rect with grid
  const QPoint gridTopLeft = (rect.topLeft() / GRID_SIZE).toPoint() * GRID_SIZE;
  const QPoint gridBotRight =
      (rect.bottomRight() / GRID_SIZE).toPoint() * GRID_SIZE;

  for (int x = gridTopLeft.x(); x <= gridBotRight.x(); x += GRID_SIZE)
    for (int y = gridTopLeft.y(); y <= gridBotRight.y(); y += GRID_SIZE)
      painter->drawPoint(x, y);

  painter->restore();
}

void VSRTLScene::setLocked(bool lock) {
  m_isLocked = lock;

  for (auto &i : items()) {
    if (auto *gb = dynamic_cast<GraphicsBase *>(i))
      gb->setLocked(lock);
  }
}

void VSRTLScene::setPortValuesVisibleForType(vsrtl::SimPort::PortType t,
                                             bool visible) {
  predicatedExecOnItems<PortGraphic>(
      [t](const PortGraphic *p) { return p->getPortType() == t; },
      &PortGraphic::setValueLabelVisible, visible);
}

void VSRTLScene::contextMenuEvent(QGraphicsSceneContextMenuEvent *event) {
  // If there are any items at the click position, forward the context event to
  // it
  if (items(event->scenePos()).size() != 0)
    return QGraphicsScene::contextMenuEvent(event);

  QMenu menu;
  // Component positioning locking
  auto lockAction = menu.addAction(m_isLocked ? "Unlock" : "Lock");
  lockAction->setCheckable(true);
  lockAction->setChecked(m_isLocked);
  connect(lockAction, &QAction::triggered, this, &VSRTLScene::setLocked);

  menu.addSeparator();

  // ==================== Port modifying actions ====================
  auto *portMenu = menu.addMenu("Ports");
  auto *showValuesAction = portMenu->addAction("Show all values");
  connect(showValuesAction, &QAction::triggered, [=] {
    this->setPortValuesVisibleForType(vsrtl::SimPort::PortType::out, true);
  });

  auto *hideValuesAction = portMenu->addAction("Hide all values");
  connect(hideValuesAction, &QAction::triggered, [=] {
    this->setPortValuesVisibleForType(vsrtl::SimPort::PortType::out, false);
  });

  auto *showWidthsAction = portMenu->addAction("Show all widths");
  connect(showWidthsAction, &QAction::triggered,
          [=] { this->setPortWidthsVisible(true); });

  auto *hideWidthsAction = portMenu->addAction("Hide all widths");
  connect(hideWidthsAction, &QAction::triggered,
          [=] { this->setPortWidthsVisible(false); });

  // ==================== Scene modifying actions ====================
  if (!m_isLocked) {
    menu.addSeparator();

    // Hidden components submenu
    auto *hiddenMenu = menu.addMenu("Hidden components");
    std::vector<QAction *> showActions;

    const auto sceneItems = items();
    for (const auto &i : std::as_const(sceneItems)) {
      if (!i->isVisible()) {
        if (auto *c = dynamic_cast<ComponentGraphic *>(i)) {
          // If a components parent is expanded but it itself is not visible,
          // then it may be set to being currently visible
          if (c->getParent()->isExpanded()) {
            auto *action = hiddenMenu->addAction(
                QString::fromStdString(c->getComponent()->getName()));
            connect(action, &QAction::triggered,
                    [c] { c->setUserVisible(true); });
            showActions.push_back(action);
          }
        }
      }
    }

    if (hiddenMenu->actions().size() != 0) {
      hiddenMenu->addSeparator();
      auto *showAllAction = hiddenMenu->addAction("Show all");
      connect(showAllAction, &QAction::triggered, [showActions] {
        for (const auto &a : showActions)
          a->trigger();
      });
    } else {
      // Disable the hidden components menu if there are no hidden components to
      // be re-enabled
      hiddenMenu->setDisabled(hiddenMenu->actions().size() == 0);
    }

    menu.addSeparator();

    // ============== Drawing options =============================
    auto *drawMenu = menu.addMenu("Drawing");
    auto *showGridAction = drawMenu->addAction("Show grid");
    showGridAction->setCheckable(true);
    showGridAction->setChecked(m_showGrid);
    connect(showGridAction, &QAction::toggled, [=](bool checked) {
      m_showGrid = checked;
      this->update();
    });

    drawMenu->addAction(m_darkmodeAction);
  }

  menu.exec(event->screenPos());
}

void VSRTLScene::setDarkmode(bool enabled) {
  m_darkmodeAction->setChecked(enabled);
}

void VSRTLScene::setPortWidthsVisible(bool visible) {
  execOnItems<PortGraphic>(&PortGraphic::setPortWidthVisible, visible);
}

void VSRTLScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
  handleWirePointMove(event);

  return QGraphicsScene::mouseMoveEvent(event);
}

void VSRTLScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
  if (m_selectedPoint && m_currentDropTargets.size() != 0) {
    auto *mergepoint = *m_currentDropTargets.begin();
    mergepoint->pointDrop(m_selectedPoint);
    mergepoint->pointDragLeave(m_selectedPoint);
    m_currentDropTargets.clear();
  }
  QGraphicsScene::mouseReleaseEvent(event);
}

void VSRTLScene::handleSelectionChanged() {
  m_selectedPoint = getSingleSelectedItem<WirePoint>(*this);
}

} // namespace vsrtl
