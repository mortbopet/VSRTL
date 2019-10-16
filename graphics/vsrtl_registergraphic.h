#ifndef VSRTL_REGISTERGRAPHIC_H
#define VSRTL_REGISTERGRAPHIC_H

#include "vsrtl_componentgraphic.h"

#include "vsrtl_register.h"

namespace vsrtl {

class RegisterGraphic : public ComponentGraphic {
public:
    RegisterGraphic(RegisterBase& c);
    void paintOverlay(QPainter* painter, const QStyleOptionGraphicsItem* item, QWidget* w) override;

private:
    RegisterBase& m_register;
};

}  // namespace vsrtl

#endif  // VSRTL_REGISTERGRAPHIC_H
