#include "vsrtl_graphicsbase.h"

namespace vsrtl {
GraphicsBase::GraphicsBase() {}

void GraphicsBase::postSceneConstructionInitialize() {
    // Propagate post scene construction initialization call to child items
    for (const auto& child : childItems()) {
        GraphicsBase* g_child = dynamic_cast<GraphicsBase*>(child);
        if (g_child) {
            g_child->postSceneConstructionInitialize();
        }
    }
}
}  // namespace vsrtl
