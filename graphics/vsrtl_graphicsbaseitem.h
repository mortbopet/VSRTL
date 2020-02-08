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

template <typename T = QGraphicsItem>
class GraphicsBaseItem : public GraphicsBase, public T {
    static_assert(std::is_base_of<QGraphicsItem, T>::value, "GraphicsBaseItem must derive from QGraphicsItem");

public:
    GraphicsBaseItem(QGraphicsItem* parent) : T(parent) {}

    void postSceneConstructionInitialize1() override {
        recurseToChildren(this, [](GraphicsBase* child) { child->postSceneConstructionInitialize1(); });
    }
    void postSceneConstructionInitialize2() override {
        recurseToChildren(this, [](GraphicsBase* child) { child->postSceneConstructionInitialize2(); });

        m_initialized = true;
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
};

}  // namespace vsrtl
