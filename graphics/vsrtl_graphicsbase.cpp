#include "vsrtl_graphicsbase.h"

namespace vsrtl {
GraphicsBase::GraphicsBase() {}

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
}  // namespace vsrtl
