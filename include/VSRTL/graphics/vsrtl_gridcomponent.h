#pragma once

#include <QMetaType>
#include <QRect>
#include <map>

#include "../interface/vsrtl_interface.h"
#include "vsrtl_componentborder.h"
#include "vsrtl_graphicsbaseitem.h"
#include "vsrtl_shape.h"
#include "vsrtl_simqobject.h"

namespace vsrtl {

class GridComponent : public SimQObject,
                      public GraphicsBaseItem<QGraphicsItem> {
  Q_OBJECT
public:
  GridComponent(SimComponent *c, GridComponent *parent);

  enum class RotationDirection { RightHand, LeftHand };
  /**
   * @brief rotate
   * Attempt to rotate the component in the direction of @p dir
   */
  void gridRotate(const RotationDirection &dir);
  int gridRotation() const { return m_gridRotation; }

  /**
   * @brief adjust
   * Attempt to expand the currently visible grid rect. Bounded by current
   * minimum rect size.
   * @return true if any change to the currently visible grid rect was performed
   */
  bool adjust(const QPoint &);
  bool adjust(const QRect &);

  /**
   * @brief move
   * Attempts to move this gridcomponent to the desired pos in the given
   * coordinate basis.
   * @return true if move was successfull (ie. if move was within bounds)
   */
  bool move(const QPoint &pos);

  /**
   * @brief setExpanded
   * Change the current expansion state to @p state
   */
  void setExpanded(bool state);

  bool isExpanded() const { return m_expanded; }

  /**
   * @brief adjustPort
   * Attempt to move the position of @p port to @p pos of its current side
   * @return  whether the requested pos has been set for the port
   */
  bool adjustPort(SimPort *port, QPoint pos);

  PortPos getPortPos(const SimPort *port) const;
  std::vector<unsigned> getFreePortPositions(Side s);

  bool parentIsPlacing() const;

  const ComponentBorder &getBorder() const { return *m_border; }
  const QRect &getCurrentComponentRect() const;
  QRect getCurrentMinRect() const;
  const QRect &getLastComponentRect() const { return m_lastComponentRect; }

  QPoint getGridPos() const { return m_relPos; }

  SimComponent *getComponent() const { return m_component; }
  bool hasSubcomponents() const;

  void placeAndRouteSubcomponents();

  template <class Archive>
  void serializeBorder(Archive &archive) {
    m_border->serialize(archive);
    // Invalidate all port positions

    for (const auto &p : m_component->getAllPorts()) {
      emit portPosChanged(p);
    }
  }

signals:
  void gridRectChanged();
  void gridPosChanged(QPoint);
  void portPosChanged(const vsrtl::SimPort *p);

protected:
  int m_gridRotation =
      0; // Rotation angle, in the range [0, 360[ in intervals of 90 deg.

  SimComponent *m_component = nullptr;
  /**
   * @brief spreadPorts
   * Adjust all ports of the component such that they are evenly spread on a
   * given face of the gridcomponent. Ports' direction will not be changed
   */
  void spreadPorts();

  /**
   * @brief spreadPortsOrdered
   * Adjust all ports of the component such that their position is evenly spread
   * on the faces of the gridcomponent. Ports are spread in the order of which
   * they currently appear on the given side.
   */
  void spreadPortsOrdered();

private:
  /**
   * @brief childGeometryChanged
   * Called by child components, signalling that their geometry or position was
   * changed, which (may) require a rezing of the current minimum component
   * rect.
   */
  void childGeometryChanged();

private:
  /**
   * @brief spreadPortsOnSide
   * Spread all ports currently located on @p side
   */
  void spreadPortsOnSide(const Side &side);

  /**
   * @brief rotatePorts
   * Rotates all ports in the provided @param dir
   */
  void rotatePorts(const RotationDirection &dir);

  /**
   * @brief Rect update functions
   */
  void updateCurrentComponentRect(int dx, int dy);
  QRect getContractedMinimumGridRect() const;
  bool updateSubcomponentBoundingRect();
  void setInitialRect();

  QRect &getCurrentComponentRectRef();

  /**
   * @brief moveInsideParent
   * Attempts to move this gridcomponent to the desired @p pos inside the
   * parent.
   * @return true if move was successfull (ie. if move was within the parent
   * bounds)
   */
  bool moveInsideParent(QPoint pos);

  bool parentContainsRect(const QRect &r) const;
  std::vector<GridComponent *> getGridSubcomponents() const;

  std::unique_ptr<ComponentBorder> m_border;

  /** current expansion state */
  bool m_expanded = false;

  /** Flag for indicating when placement/routing. If so, do not restrict
   * subcomponent positioning inside the current minimum subcomponent bounding
   * rect */
  bool m_isPlacing = false;

  /// Managed rectangles
  QRect m_currentExpandedRect;
  QRect m_currentContractedRect;
  QRect m_currentSubcomponentBoundingRect;
  QRect m_lastComponentRect =
      QRect(); // whenever the current component rect changes, objects such as
               // component labels etc. may need to be adjusted based on the
               // scaled difference between the new and old visible grid rects.
               // For this, we cache the last updated grid rect.

  QPoint m_relPos; // Position in parent coordinates
};

} // namespace vsrtl
