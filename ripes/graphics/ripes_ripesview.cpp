#include "ripes_ripesview.h"

#include <QSlider>
#include <QWheelEvent>

#include <qmath.h>

namespace ripes {

RipesView::RipesView(QWidget* parent) : QGraphicsView(parent) {
    m_zoom = 250;

    setDragMode(QGraphicsView::RubberBandDrag);
    setInteractive(true);
}

void RipesView::wheelEvent(QWheelEvent* e) {
    if (e->modifiers() & Qt::ControlModifier) {
        if (e->delta() > 0)
            zoomIn(6);
        else
            zoomOut(6);
        e->accept();
    } else {
        QGraphicsView::wheelEvent(e);
    }
}

void RipesView::zoomIn(int level) {
    m_zoom += level;
    setupMatrix();
}

void RipesView::zoomOut(int level) {
    m_zoom -= level;
    setupMatrix();
}

void RipesView::setupMatrix() {
    qreal scale = qPow(qreal(2), (m_zoom - 250) / qreal(50));

    QMatrix matrix;
    matrix.scale(scale, scale);

    setMatrix(matrix);
}
}
