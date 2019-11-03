#ifndef VSRTL_GRAPHICSBASE_H
#define VSRTL_GRAPHICSBASE_H

#include <QGraphicsItem>

namespace vsrtl {

class GraphicsBase : public QObject, public QGraphicsItem {
public:
    GraphicsBase(QGraphicsItem* parent);

    /**
     * @brief postSceneConstructionInitialize#
     * Some graphic components may need extra initialization steps after all graphics have been added to the scene (such
     * as wires). When overriding, overriding function must call GraphicsBase::postSceneConstructionInitialize#()
     *
     * Multiple passes may be used, allowing for staged initialization
     */
    virtual void postSceneConstructionInitialize1();
    virtual void postSceneConstructionInitialize2();

    /**
     * @brief setLocked
     * Toggles any interaction with the object in the scene. Components may specialize, if further modifications to the
     * components behaviour is required
     */
    virtual void setLocked(bool locked) {
        if (!m_isMoveable)
            return;

        if (locked)
            setFlags(flags() & ~QGraphicsItem::ItemIsMovable);
        else
            setFlag(QGraphicsItem::ItemIsMovable);
    }

    void setMoveable() { m_isMoveable = true; }
    bool isLocked() const;

protected:
    bool m_initialized = false;
    bool m_isMoveable = false;
};
}  // namespace vsrtl

#endif  // VSRTL_GRAPHICSBASE_H
