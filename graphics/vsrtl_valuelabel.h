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
    void mouseMoveEvent(QGraphicsSceneMouseEvent*) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent*) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent*) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    void updateText() override;

    void setLocked(bool locked) override;

private:
    void createLineToPort();
    QLineF lineToPort() const;
    void updateLine();

    bool m_alwaysShowLineToPort = false;
    std::shared_ptr<Radix> m_radix;
    const PortGraphic* m_port = nullptr;
    QGraphicsLineItem* m_lineToPort = nullptr;
};

}  // namespace vsrtl
