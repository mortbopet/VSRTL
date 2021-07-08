#pragma once

#include "vsrtl_label.h"
#include "vsrtl_radix.h"

#include "../interface/vsrtl_interface.h"

#include <QGraphicsItem>

namespace vsrtl {

class PortGraphic;

class ValueLabel : public Label {
public:
    ValueLabel(QGraphicsItem* parent, const std::shared_ptr<Radix>& radix, const PortGraphic* port);

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* item, QWidget*) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent*) override;

    void updateText() override;
    void setLocked(bool locked) override;

private:
    std::shared_ptr<Radix> m_radix;
    const PortGraphic* m_port = nullptr;
};

}  // namespace vsrtl
