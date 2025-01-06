#ifndef VSRTL_GRAPHICSBASE_H
#define VSRTL_GRAPHICSBASE_H

#include "gallantsignalwrapper.h"
#include <QGraphicsItem>

namespace vsrtl {

class GraphicsBase {
public:
  /**
   * @brief The VirtualChildLink struct
   * Defines which changes to mirror in the virtual child
   */
  enum VirtualChildLink { Position = 0b1, Visibility = 0b10 };
  Q_DECLARE_FLAGS(VirtualChildLinks, VirtualChildLink);

  GraphicsBase() {}
  virtual ~GraphicsBase() {}

  /**
   * @brief postSceneConstructionInitialize#
   * Some graphic components may need extra initialization steps after all
   * graphics have been added to the scene (such as wires). When overriding,
   * overriding function must call
   * GraphicsBase::postSceneConstructionInitialize#()
   *
   * Multiple passes may be used, allowing for staged initialization
   */
  virtual void postSceneConstructionInitialize1() = 0;
  virtual void postSceneConstructionInitialize2() = 0;

  /**
   * @brief setLocked
   * Toggles any interaction with the object in the scene. Components may
   * specialize, if further modifications to the components behaviour is
   * required
   */
  virtual void setLocked(bool locked) = 0;

  void setMoveable(bool moveable = true) {
    m_isMoveable = moveable;
    setLocked(!moveable);
  }
  virtual bool isLocked() const = 0;

  virtual void setSerializing(bool state) = 0;
  bool isSerializing() const { return m_isSerializing; }

  void addVirtualChild(const VirtualChildLinks &link, GraphicsBase *child) {
    Q_ASSERT(m_virtualChildren.count(child) == 0);
    m_virtualChildren[child] = link;
    child->m_virtualParents[this] = link;
  }

  /**
   * @brief m_virtualChildren
   * Virtual children are items which have no QGraphicsItem child/parent
   * relationship to this item, but who should mirror position and visibility
   * changes made to this object.
   */
  std::map<GraphicsBase *, VirtualChildLinks> m_virtualChildren;
  std::map<GraphicsBase *, VirtualChildLinks> m_virtualParents;

  bool m_initialized = false;
  bool m_isMoveable = false;

  /** Flag for indicating when serializing this component. When active,
   * GraphicsBase derived objects may temporarily disable various
   * runtime-enabled checks which will could deserialization. */
  bool m_isSerializing = false;
};

} // namespace vsrtl

#endif // VSRTL_GRAPHICSBASE_H
