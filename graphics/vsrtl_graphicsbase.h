#ifndef VSRTL_GRAPHICSBASE_H
#define VSRTL_GRAPHICSBASE_H

#include <QGraphicsItem>

namespace vsrtl {

class GraphicsBase {
public:
    GraphicsBase() {}
    virtual ~GraphicsBase() {}

    /**
     * @brief postSceneConstructionInitialize#
     * Some graphic components may need extra initialization steps after all graphics have been added to the scene (such
     * as wires). When overriding, overriding function must call GraphicsBase::postSceneConstructionInitialize#()
     *
     * Multiple passes may be used, allowing for staged initialization
     */
    virtual void postSceneConstructionInitialize1() = 0;
    virtual void postSceneConstructionInitialize2() = 0;

    /**
     * @brief setLocked
     * Toggles any interaction with the object in the scene. Components may specialize, if further modifications to the
     * components behaviour is required
     */
    virtual void setLocked(bool locked) = 0;

    void setMoveable() {
        m_isMoveable = true;
        setLocked(false);
    }
    virtual bool isLocked() const = 0;

    virtual void setSerializing(bool state) = 0;
    bool isSerializing() const { return m_isSerializing; }

protected:
    bool m_initialized = false;
    bool m_isMoveable = false;

    /** Flag for indicating when serializing this component. When active, GraphicsBase derived objects may
     * temporarily disable various runtime-enabled checks which will could deserialization. */
    bool m_isSerializing = false;
};

}  // namespace vsrtl

#endif  // VSRTL_GRAPHICSBASE_H
