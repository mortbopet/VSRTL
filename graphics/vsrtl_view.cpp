#include "vsrtl_view.h"

#include <QSlider>
#include <QWheelEvent>

#include "math.h"

namespace vsrtl {

VSRTLView::VSRTLView(QWidget* parent) : QGraphicsView(parent) {
    setDragMode(QGraphicsView::RubberBandDrag);
    setOptimizationFlag(QGraphicsView::DontSavePainterState);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setRenderHint(QPainter::Antialiasing, false);
    setInteractive(true);
    setupMatrix();
}

ComponentGraphic* VSRTLView::lookupGraphicForComponent(const SimComponent* c) {
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

void VSRTLView::zoomToFit(const QGraphicsItem* tlc) {
    fitInView(tlc->boundingRect(), Qt::KeepAspectRatio);
    const auto m = matrix();

    // When zooming to fit, we want to adjust the current view via. fitInView, and then derive the zoom factor
    // corresponding to the scale factor which was set by fitInView.

    if (m.m11() < 0.1) {
        // There is some strange bug in the QGraphicsScene/QGraphicsView logic which does not calculate the bounding
        // matrix of the top level component correctly, if the qgraphicsview is not currently visible. In this case,
        // revert to the deafult zoom amount if some too small zoom is detected when trying to fit into view.
        m_zoom = s_zoomDefault;
    } else {
        m_zoom = std::log2(matrix().m11()) * s_zoomScale + s_zoomDefault;
    }

    setupMatrix();
}

void VSRTLView::wheelEvent(QWheelEvent* e) {
    if (e->modifiers() & Qt::ControlModifier) {
        if (e->delta() > 0)
            zoomIn(s_zoomInterval);
        else
            zoomOut(s_zoomInterval);
        e->accept();
    } else {
        QGraphicsView::wheelEvent(e);
    }
}

void VSRTLView::zoomIn(double level) {
    m_zoom += level;
    setupMatrix();
}

void VSRTLView::zoomOut(double level) {
    m_zoom -= level;
    setupMatrix();
}

void VSRTLView::setupMatrix() {
    const double scale = std::pow(2.0, (m_zoom - s_zoomDefault) / s_zoomScale);

    QMatrix matrix;
    matrix.scale(scale, scale);

    setMatrix(matrix);
}
}  // namespace vsrtl
