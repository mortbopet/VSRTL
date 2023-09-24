#pragma once

#include "vsrtl_graphicsbase.h"
#include "vsrtl_scene.h"
#include <QGraphicsItem>

namespace vsrtl {

template <typename T>
class GraphicsBaseItem;

template <typename F, typename T = QGraphicsItem>
void recurseToChildren(GraphicsBaseItem<T> *parent, const F &func) {
  // Propagate post scene construction initialization call to child items
  for (const auto &child : parent->childItems()) {
    GraphicsBase *g_child = dynamic_cast<GraphicsBase *>(child);
    if (g_child) {
      func(g_child);
    }
  }
}

/**
 * Base type can be any QGraphicsItem derived type. This is useful if leveraging
 * already available items such as QGraphicsTextItem etc..
 */
template <typename T = QGraphicsItem>
class GraphicsBaseItem : public GraphicsBase, public T {
  static_assert(std::is_base_of<QGraphicsItem, T>::value,
                "GraphicsBaseItem must derive from QGraphicsItem");

public:
  GraphicsBaseItem(QGraphicsItem *parent) : T(parent) {
    T::setFlag(QGraphicsItem::ItemSendsGeometryChanges);
    // Always ensure that this is off - scene position changes absolutely kills
    // performance when we have many nested components.
    T::setFlag(QGraphicsItem::ItemSendsScenePositionChanges, false);
  }

  void postSceneConstructionInitialize1() override {
    recurseToChildren(this, [](GraphicsBase *child) {
      child->postSceneConstructionInitialize1();
    });
  }
  void postSceneConstructionInitialize2() override {
    recurseToChildren(this, [](GraphicsBase *child) {
      child->postSceneConstructionInitialize2();
    });

    m_initialized = true;
  }

  /**
   * @brief moduleParent
   * @return the first SimComponent which encloses this GraphicsBaseItem
   */
  virtual GraphicsBaseItem<QGraphicsItem> *moduleParent() {
    auto *parent =
        dynamic_cast<GraphicsBaseItem<QGraphicsItem> *>(T::parentItem());
    Q_ASSERT(parent);
    return parent->moduleParent();
  }

  template <typename T_C, typename... Args>
  T_C *createModuleChild(const VirtualChildLinks &link, Args... args) {
    auto *parent = moduleParent();
    Q_ASSERT(parent);
    auto ptr = new T_C(parent, args...);
    // Set initial position to (0,0) in this components coordinate system
    const auto p = T::mapToItem(parent, QPointF(0, 0));
    ptr->setPos(p);
    addVirtualChild(link, ptr);
    return ptr;
  }

  void setLocked(bool locked) override {
    if (!m_isMoveable)
      return;

    if (locked)
      T::setFlags(T::flags() & ~QGraphicsItem::ItemIsMovable);
    else
      T::setFlag(QGraphicsItem::ItemIsMovable);
  }

  bool isLocked() const override {
    if (auto *p = dynamic_cast<VSRTLScene *>(T::scene())) {
      return p->isLocked();
    } else {
      return false;
    }
  }

  void setSerializing(bool state) override {
    m_isSerializing = state;
    // Recursively propagate serialize state to children
    for (const auto &c : T::childItems()) {
      if (auto *gb = dynamic_cast<GraphicsBase *>(c)) {
        gb->setSerializing(state);
      }
    }
  }

  virtual uint32_t layoutVersion() const {
    auto *parent =
        dynamic_cast<GraphicsBaseItem<QGraphicsItem> *>(T::parentItem());
    Q_ASSERT(parent);
    return parent->layoutVersion();
  }

  QVariant itemChange(QGraphicsItem::GraphicsItemChange change,
                      const QVariant &value) override {
    Q_ASSERT((T::flags() & QGraphicsItem::ItemSendsScenePositionChanges) == 0 &&
             "ItemSendsScenePositionChanges should never be enabled - kills "
             "performance");
    const auto curPos = T::pos();
    const auto dp = curPos - m_prePos;

    // Propagate completed state changes to virtual children
    if ((change == QGraphicsItem::ItemPositionHasChanged) ||
        (change == QGraphicsItem::ItemVisibleHasChanged)) {
      // Propagate geometry state changes to virtual children
      if (m_virtualChildren.size() != 0) {
        const bool vis = T::isVisible();
        for (auto &vt : m_virtualChildren) {
          if (change == QGraphicsItem::ItemPositionHasChanged &&
              vt.second.testFlag(Position)) {
            dynamic_cast<QGraphicsItem *>(vt.first)->moveBy(dp.x(), dp.y());
          } else if (change == QGraphicsItem::ItemVisibleHasChanged &&
                     vt.second.testFlag(Visibility)) {
            dynamic_cast<QGraphicsItem *>(vt.first)->setVisible(vis);
          }
        }
      }
    }
    m_prePos = curPos;

    // Decide state change based on virtual parent status
    if (!isSerializing() && (change == QGraphicsItem::ItemVisibleChange)) {
      bool virtualParentsVisible = true;
      for (const auto &vp : m_virtualParents) {
        if (vp.second.testFlag(Visibility)) {
          virtualParentsVisible &=
              dynamic_cast<QGraphicsItem *>(vp.first)->isVisible();
        }
      }
      if (!virtualParentsVisible) {
        // reject visibility change event
        return QVariant();
      }
    }

    // Handle Z level based on selection state
    if (change == QGraphicsItem::ItemSelectedChange) {
      if (value.toBool()) {
        m_preZLayer = T::zValue();
        T::setZValue(VSRTLScene::Z_Selected);
      } else {
        T::setZValue(m_preZLayer);
      }
    }

    return QGraphicsItem::itemChange(change, value);
  }

  /**
   * @brief modulePositionChanged
   * This function adds the capabilities of
   * QGraphicsItem::ItemScenePositionHasChanged restricted to within the scope
   * of a module and its direct children, drawn inside the module. By avoiding
   * to use the itemScenePos change infrastructure, we gain a massive speedup,
   * while still being able to notify the correct entities that module position
   * has changed.
   */
  virtual void modulePositionHasChanged() {}

protected:
  // State-change preservation needed for Item#HasChanged value difference
  // calculations
  QPointF m_prePos;

private:
  // Buffer Z-layer value in between selection state changes
  qreal m_preZLayer;
};

} // namespace vsrtl
