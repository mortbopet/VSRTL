#include "vsrtl_graphicsbase.h"
#include "vsrtl_scene.h"

namespace vsrtl {
GraphicsBase::GraphicsBase(QGraphicsItem* parent) : QGraphicsItem(parent) {}

template <typename F>
void recurseToChildren(GraphicsBase* parent, const F& func) {
    // Propagate post scene construction initialization call to child items
    for (const auto& child : parent->childItems()) {
        GraphicsBase* g_child = dynamic_cast<GraphicsBase*>(child);
        if (g_child) {
            func(g_child);
        }
    }
}

void GraphicsBase::postSceneConstructionInitialize1() {
    recurseToChildren(this, [](GraphicsBase* child) { child->postSceneConstructionInitialize1(); });
}
void GraphicsBase::postSceneConstructionInitialize2() {
    recurseToChildren(this, [](GraphicsBase* child) { child->postSceneConstructionInitialize2(); });

    m_initialized = true;
}

bool GraphicsBase::isLocked() const {
    auto* p = dynamic_cast<VSRTLScene*>(scene());
    Q_ASSERT(p);
    return p->isLocked();
}

}  // namespace vsrtl
