#ifndef VSRTL_SCENE_H
#define VSRTL_SCENE_H

#include <QGraphicsScene>
#include <QPainter>
#include "vsrtl_graphics_defines.h"

namespace vsrtl {

class VSRTLScene : public QGraphicsScene {
public:
    VSRTLScene(QObject* parent = nullptr) : QGraphicsScene(parent) {}

protected:
#ifdef VSRTL_DEBUG_DRAW
    void drawBackground(QPainter* painter, const QRectF& rect) override {
        qreal left = int(rect.left()) - (int(rect.left()) % GRID_SIZE);
        qreal top = int(rect.top()) - (int(rect.top()) % GRID_SIZE);

        QVarLengthArray<QLineF, 100> lines;

        for (qreal x = left; x < rect.right(); x += GRID_SIZE)
            lines.append(QLineF(x, rect.top(), x, rect.bottom()));
        for (qreal y = top; y < rect.bottom(); y += GRID_SIZE)
            lines.append(QLineF(rect.left(), y, rect.right(), y));

        painter->drawLines(lines.data(), lines.size());
    }
#endif
};

}  // namespace vsrtl

#endif  // VSRTL_SCENE_H
