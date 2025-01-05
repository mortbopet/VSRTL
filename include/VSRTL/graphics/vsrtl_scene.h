#ifndef VSRTL_SCENE_H
#define VSRTL_SCENE_H

#include <QAction>
#include <QGraphicsScene>
#include <QPainter>

#include "../interface/vsrtl_interface.h"
#include "vsrtl_graphics_defines.h"

#include <functional>
#include <set>

namespace vsrtl {
class WirePoint;

class VSRTLScene : public QGraphicsScene {
public:
  /**
   * @brief The ZLayer enum
   * Defines the ordering of different items in the scene. Items with higher Z
   * values will be drawn above items with lower Z values.
   */
  enum ZLayer {
    Z_Wires,
    Z_Component,
    Z_PortWidth,
    Z_PortLabel,
    Z_ValueLabelHoverLine,
    Z_ValueLabel,
    Z_Selected
  };
  VSRTLScene(QObject *parent = nullptr);

  void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
  void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
  void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;
  void drawBackground(QPainter *painter, const QRectF &rect) override;

  void setPortValuesVisibleForType(vsrtl::SimPort::PortType t, bool visible);
  void setPortWidthsVisible(bool visible);
  void setLocked(bool locked);

  bool isLocked() const { return m_isLocked; }
  bool darkmode() const { return m_darkmode; }
  void setDarkmode(bool enabled);

private:
  void handleSelectionChanged();
  void handleWirePointMove(QGraphicsSceneMouseEvent *event);

  bool m_darkmode = false;
  bool m_showGrid = true;
  std::set<WirePoint *> m_currentDropTargets;
  WirePoint *m_selectedPoint = nullptr;
  QAction *m_darkmodeAction = nullptr;

  /**
   * @brief m_isLocked
   * When set, components all interaction with objects in the scene beyond
   * changing the view style of signal values is disabled.
   */
  bool m_isLocked = false;

  /* Applies T::F to all items in the scene of type F, using the supplied
   * arguments */
  template <typename T, typename F, typename... Args>
  void execOnItems(F f, Args... args) {
    const auto sceneItems = items();
    for (auto *c : std::as_const(sceneItems)) {
      if (auto *t_c = dynamic_cast<T *>(c)) {
        (t_c->*f)(args...);
      }
    }
  }

  /* Applies T::F to all items in the scene of type F, using the supplied
   * arguments, if predicate returns true */
  template <typename T, typename F, typename... Args>
  void predicatedExecOnItems(std::function<bool(const T *)> pred, F &&f,
                             Args... args) {
    const auto sceneItems = items();
    for (auto *c : std::as_const(sceneItems)) {
      if (auto *t_c = dynamic_cast<T *>(c)) {
        if (pred(t_c)) {
          (t_c->*f)(args...);
        }
      }
    }
  }
};

} // namespace vsrtl

#endif // VSRTL_SCENE_H
