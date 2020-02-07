#pragma once

#include "vsrtl_label.h"
#include "vsrtl_radix.h"

#include "../interface/vsrtl_interface.h"

#include <QGraphicsItem>

namespace vsrtl {

class ValueLabel : public Label {
public:
    ValueLabel(Radix& type, const SimPort* port, QGraphicsItem* parent);

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* item, QWidget*) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

    void updateText();
    void setLocked(bool locked) override;

private:
    Radix& m_type;
    const SimPort* m_port = nullptr;
    unsigned int m_maxBitWidth;
};

}  // namespace vsrtl
