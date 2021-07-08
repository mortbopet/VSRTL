#pragma once

#include <QGraphicsItem>
#include "vsrtl_graphicsbase.h"
#include "vsrtl_scene.h"

namespace vsrtl {

template <typename T>
class GraphicsBaseItem;

template <typename F, typename T = QGraphicsItem>
void recurseToChildren(GraphicsBaseItem<T>* parent, const F& func) {
    // Propagate post scene construction initialization call to child items
    for (const auto& child : parent->childItems()) {
        GraphicsBase* g_child = dynamic_cast<GraphicsBase*>(child);
        if (g_child) {
            func(g_child);
        }
    }
}

/**
 * Base type can be any QGraphicsItem derived type. This is useful if leveraging already available items such as
 * QGraphicsTextItem etc..
 */
template <typename T = QGraphicsItem>
class GraphicsBaseItem : public GraphicsBase, public T {
    static_assert(std::is_base_of<QGraphicsItem, T>::value, "GraphicsBaseItem must derive from QGraphicsItem");

public:
    GraphicsBaseItem(QGraphicsItem* parent) : T(parent) { T::setFlag(QGraphicsItem::ItemSendsGeometryChanges); }

    void postSceneConstructionInitialize1() override {
        recurseToChildren(this, [](GraphicsBase* child) { child->postSceneConstructionInitialize1(); });
    }
    void postSceneConstructionInitialize2() override {
        recurseToChildren(this, [](GraphicsBase* child) { child->postSceneConstructionInitialize2(); });

        m_initialized = true;
    }

    /**
     * @brief moduleParent
     * @return the first SimComponent which encloses this GraphicsBaseItem
     */
    virtual GraphicsBaseItem<QGraphicsItem>* moduleParent() {
        auto* parent = dynamic_cast<GraphicsBaseItem<QGraphicsItem>*>(T::parentItem());
        Q_ASSERT(parent);
        return parent->moduleParent();
    }

    /**
     * @brief The VirtualChildLink struct
     * Defines which changes to mirror in the virtual child
     */
    enum VirtualChildLink { Position = 0b1, Visibility = 0b10 };
    Q_DECLARE_FLAGS(VirtualChildLinks, VirtualChildLink);

    template <typename T_C, typename... Args>
    T_C* createVirtualChild(const VirtualChildLinks& link, Args... args) {
        auto* parent = moduleParent();
        Q_ASSERT(parent);
        auto ptr = new T_C(parent, args...);
        // Set initial position to (0,0) in this components coordinate system
        const auto p = T::mapToItem(parent, QPointF(0, 0));
        ptr->setPos(p);
        addVirtualChild(link, ptr);
        return ptr;
    }

    void addVirtualChild(const VirtualChildLinks& link, QGraphicsItem* child) {
        Q_ASSERT(m_virtualChildren.count(child) == 0);
        m_virtualChildren[child] = link;
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
        auto* p = dynamic_cast<VSRTLScene*>(T::scene());
        Q_ASSERT(p);
        return p->isLocked();
    }

    void setSerializing(bool state) override {
        m_isSerializing = state;
        // Recursively propagate serialize state to children
        for (const auto& c : T::childItems()) {
            if (auto* gb = dynamic_cast<GraphicsBase*>(c)) {
                gb->setSerializing(state);
            }
        }
    }

    virtual uint32_t layoutVersion() const {
        auto* parent = dynamic_cast<GraphicsBaseItem<QGraphicsItem>*>(T::parentItem());
        Q_ASSERT(parent);
        return parent->layoutVersion();
    }

    QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant& value) override {
        const auto curPos = T::pos();
        const auto dp = curPos - m_prePos;
        if (!isSerializing() &&
            ((change == QGraphicsItem::ItemPositionHasChanged) || (change == QGraphicsItem::ItemVisibleHasChanged))) {
            // Propagate geometry state changes to virtual children
            if (m_virtualChildren.size() != 0) {
                const bool vis = T::isVisible();
                for (const auto& vt : m_virtualChildren) {
                    if (change == QGraphicsItem::ItemPositionHasChanged && vt.second.testFlag(Position)) {
                        vt.first->moveBy(dp.x(), dp.y());
                    } else if (change == QGraphicsItem::ItemVisibleHasChanged && vt.second.testFlag(Visibility)) {
                        vt.first->setVisible(vis);
                    }
                }
            }
        }
        m_prePos = curPos;

        return QGraphicsItem::itemChange(change, value);
    }

protected:
    /**
     * @brief m_virtualChildren
     * Virtual children are items which have no QGraphicsItem child/parent relationship to this item, but who should
     * mirror position and visibility changes made to this object.
     */
    std::map<QGraphicsItem*, VirtualChildLinks> m_virtualChildren;

    // State-change preservation needed for Item#HasChanged value difference calculations
    QPointF m_prePos;
};

}  // namespace vsrtl
