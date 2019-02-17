#ifndef VSRTL_MULTIPLEXERGRAPHIC_H
#define VSRTL_MULTIPLEXERGRAPHIC_H

#include "vsrtl_componentgraphic.h"

#include "vsrtl_multiplexer.h"

namespace vsrtl {

class MultiplexerGraphic : public ComponentGraphic {
public:
    MultiplexerGraphic(Multiplexer& c);
    void paintOverlay(QPainter* painter, const QStyleOptionGraphicsItem* item, QWidget* w) override;

private:
    Multiplexer& m_multiplexer;
};

}  // namespace vsrtl

#endif  // VSRTL_MULTIPLEXERGRAPHIC_H
