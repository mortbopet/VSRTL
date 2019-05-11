#ifndef VSRTL_GRAPHICS_UTIL_H
#define VSRTL_GRAPHICS_UTIL_H

#include "vsrtl_graphics_defines.h"

#include <QRect>
#include <cmath>

namespace vsrtl {

static inline QRectF gridToScene(const QRect& gridRect) {
    // Scales a rectangle in grid coordinates to scene coordinates. -1 because QRect returns width and height offset by
    // 1 for "historical reasons"
    QRectF sceneGridRect;
    sceneGridRect.moveTo(gridRect.topLeft() * GRID_SIZE);
    sceneGridRect.setWidth((gridRect.width() - 1) * GRID_SIZE);
    sceneGridRect.setHeight((gridRect.height() - 1) * GRID_SIZE);
    return sceneGridRect;
}

static inline QRect sceneToGrid(QRectF sceneRect) {
    // Scales a rectangle in scene coordinates to grid coordinates
    sceneRect.setWidth(sceneRect.width() / GRID_SIZE);
    sceneRect.setHeight(sceneRect.height() / GRID_SIZE);

    // When converting to integer-based grid rect, round up to ensure all components are inside
    return QRect(QPoint(0, 0), QSize(static_cast<int>(std::ceil(sceneRect.width())),
                                     static_cast<int>(std::ceil(sceneRect.height()))));
}

}  // namespace vsrtl

#endif  // VSRTL_GRAPHICS_UTIL_H
