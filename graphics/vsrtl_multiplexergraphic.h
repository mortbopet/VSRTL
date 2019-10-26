#ifndef VSRTL_MULTIPLEXERGRAPHIC_H
#define VSRTL_MULTIPLEXERGRAPHIC_H

#include "vsrtl_componentgraphic.h"

#include "vsrtl_multiplexer.h"

namespace vsrtl {

class MultiplexerGraphic : public ComponentGraphic {
public:
    MultiplexerGraphic(MultiplexerBase& c, QGraphicsItem* parent);
    void paintOverlay(QPainter* painter, const QStyleOptionGraphicsItem* item, QWidget* w) override;

private:
    MultiplexerBase& m_multiplexer;
};

}  // namespace vsrtl

#endif  // VSRTL_MULTIPLEXERGRAPHIC_H
