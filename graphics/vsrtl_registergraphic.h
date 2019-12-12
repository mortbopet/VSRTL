#ifndef VSRTL_REGISTERGRAPHIC_H
#define VSRTL_REGISTERGRAPHIC_H

#include "vsrtl_componentgraphic.h"

namespace vsrtl {

class RegisterGraphic : public ComponentGraphic {
public:
    RegisterGraphic(SimComponent* c, QGraphicsItem* parent);
    void paintOverlay(QPainter* painter, const QStyleOptionGraphicsItem* item, QWidget* w) override;
};

}  // namespace vsrtl

#endif  // VSRTL_REGISTERGRAPHIC_H
