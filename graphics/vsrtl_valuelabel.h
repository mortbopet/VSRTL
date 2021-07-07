#pragma once

#include "vsrtl_label.h"
#include "vsrtl_radix.h"

#include "../interface/vsrtl_interface.h"

#include <QGraphicsItem>

namespace vsrtl {

class PortGraphic;

class ValueLabel : public Label {
public:
    ValueLabel(QGraphicsItem* parent, Radix& type, const PortGraphic* port);

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* item, QWidget*) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent*) override;

    void updateText() override;
    void setLocked(bool locked) override;

private:
    Radix& m_type;
    const PortGraphic* m_port = nullptr;
};

}  // namespace vsrtl
