#include "VSRTL/graphics/vsrtl_componentbutton.h"

namespace vsrtl {

ComponentButton::ComponentButton(QGraphicsItem *parentItem)
    : QGraphicsObject(parentItem) {
  setFlags(ItemIsSelectable);
}

} // namespace vsrtl