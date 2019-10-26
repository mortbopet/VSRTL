#include "vsrtl_view.h"

#include <QSlider>
#include <QWheelEvent>

#include <qmath.h>

namespace vsrtl {

VSRTLView::VSRTLView(QWidget* parent) : QGraphicsView(parent) {
    m_zoom = 250;

    setDragMode(QGraphicsView::RubberBandDrag);
    setOptimizationFlag(QGraphicsView::DontSavePainterState);
    setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setCacheMode(QGraphicsView::CacheBackground);
    setInteractive(true);

    // Setup rendering widget
    m_renderer = new QOpenGLWidget(this);
    QSurfaceFormat f;
    f.setSamples(5);
    m_renderer->setFormat(f);
    setViewport(m_renderer);
}

ComponentGraphic* VSRTLView::lookupGraphicForComponent(const Component* c) {
    for (const auto& i : items()) {
        auto d = dynamic_cast<ComponentGraphic*>(i);
        if (d) {
            // Equality is based on pointer equality
            if (d->getComponent() == c)
                return d;
        }
    }
    return nullptr;
}

void VSRTLView::wheelEvent(QWheelEvent* e) {
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

void VSRTLView::zoomIn(int level) {
    m_zoom += level;
    setupMatrix();
}

void VSRTLView::zoomOut(int level) {
    m_zoom -= level;
    setupMatrix();
}

void VSRTLView::setupMatrix() {
    qreal scale = qPow(qreal(2), (m_zoom - 250) / qreal(50));

    QMatrix matrix;
    matrix.scale(scale, scale);

    setMatrix(matrix);
}
}  // namespace vsrtl
