#ifndef VSRTL_GRAPHICSBASE_H
#define VSRTL_GRAPHICSBASE_H

#include <QGraphicsItem>

namespace vsrtl {

class GraphicsBase : public QGraphicsItem {
public:
    GraphicsBase();

    /**
     * @brief postSceneConstructionInitialize
     * Some graphic components may need extra initialization steps after all graphics have been added to the scene (such
     * as wires). When overriding, overriding function must call GraphicsBase::postSceneConstructionInitialize()
     */
    virtual void postSceneConstructionInitialize();
};
}  // namespace vsrtl

#endif  // VSRTL_GRAPHICSBASE_H
