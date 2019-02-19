#ifndef VSRTL_WIREGRAPHIC_H
#define VSRTL_WIREGRAPHIC_H

#include "vsrtl_graphicsbase.h"

namespace vsrtl {

class PortGraphic;
class PortBase;

class WireGraphic : public GraphicsBase {
    friend class PortGraphic;

public:
    WireGraphic(PortGraphic* from, const std::vector<PortBase*>& to, QGraphicsItem* parent);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* item, QWidget*) override;

    void postSceneConstructionInitialize() override;

    PortGraphic* getFromPort() const { return m_fromPort; }

private:
    PortGraphic* m_fromPort;
    const std::vector<PortBase*>& m_toPorts;
    std::vector<PortGraphic*> m_toGraphicPorts;
};
}  // namespace vsrtl

#endif  // VSRTL_WIREGRAPHIC_H
