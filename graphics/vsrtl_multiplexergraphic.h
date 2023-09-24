#ifndef VSRTL_MULTIPLEXERGRAPHIC_H
#define VSRTL_MULTIPLEXERGRAPHIC_H

#include "vsrtl_componentgraphic.h"

#include "vsrtl_multiplexer.h"

namespace vsrtl {

class MultiplexerGraphic : public ComponentGraphic {
public:
  MultiplexerGraphic(SimComponent *c, ComponentGraphic *parent);
  void paintOverlay(QPainter *painter, const QStyleOptionGraphicsItem *item,
                    QWidget *w) override;

private:
  SimPort *getSelect();
};

} // namespace vsrtl

#endif // VSRTL_MULTIPLEXERGRAPHIC_H
