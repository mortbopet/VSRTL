#include "vsrtl_view.h"

#include <QMouseEvent>
#include <QResizeEvent>
#include <QScrollBar>
#include <QWheelEvent>

#include <algorithm>

#include "math.h"

namespace vsrtl {

VSRTLView::VSRTLView(QWidget *parent) : QGraphicsView(parent) {
  setDragMode(QGraphicsView::RubberBandDrag);
  setOptimizationFlag(QGraphicsView::DontSavePainterState);
  setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
  setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
  setRenderHint(QPainter::Antialiasing, false);
  setInteractive(true);
  setupMatrix();
}

ComponentGraphic *VSRTLView::lookupGraphicForComponent(const SimComponent *c) {
  const auto sceneItems = items();
  for (auto *i : std::as_const(sceneItems)) {
    auto d = dynamic_cast<ComponentGraphic *>(i);
    if (d) {
      // Equality is based on pointer equality
      if (d->getComponent() == c)
        return d;
    }
  }
  return nullptr;
}

void VSRTLView::zoomToFit(const QGraphicsItem *tlc) {
  if (tlc == nullptr) {
    return;
  }

  fitInView(tlc->boundingRect(), Qt::KeepAspectRatio);
  const auto m = transform();

  // When zooming to fit, we want to adjust the current view via. fitInView, and
  // then derive the zoom factor corresponding to the scale factor which was set
  // by fitInView.

  if (m.m11() < 0.1) {
    // There is some strange bug in the QGraphicsScene/QGraphicsView logic which
    // does not calculate the bounding matrix of the top level component
    // correctly, if the qgraphicsview is not currently visible. In this case,
    // revert to the deafult zoom amount if some too small zoom is detected when
    // trying to fit into view.
    m_zoom = s_zoomDefault;
  } else {
    m_zoom = std::log2(transform().m11()) * s_zoomScale + s_zoomDefault;
  }

  setupMatrix();
  ensureNavigableSceneRect();
}

void VSRTLView::wheelEvent(QWheelEvent *e) {
  if (e->modifiers() & Qt::ControlModifier) {
    if (e->angleDelta().y() > 0)
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

  QTransform matrix;
  matrix.scale(scale, scale);

  setTransform(matrix);
}

void VSRTLView::ensureNavigableSceneRect() {
  if (scene() == nullptr)
    return;

  const QRectF content = scene()->itemsBoundingRect();
  const QRectF viewportScene = mapToScene(viewport()->rect()).boundingRect();
  QRectF wanted = content.united(viewportScene);

  // Extend by (at least) one content/viewport span in each direction so the
  // view can always be panned past its content and the content can be centered.
  const qreal marginX = std::max(content.width(), viewportScene.width());
  const qreal marginY = std::max(content.height(), viewportScene.height());
  wanted.adjust(-marginX, -marginY, marginX, marginY);

  const QRectF current = sceneRect();
  if (!current.contains(wanted)) {
    // Preserve the current view center across the scene rect change to avoid
    // any visual jump while panning.
    const QPointF center = mapToScene(viewport()->rect().center());
    setSceneRect(current.isEmpty() ? wanted : current.united(wanted));
    centerOn(center);
  }
}

void VSRTLView::mousePressEvent(QMouseEvent *e) {
  const bool startPan =
      e->button() == Qt::MiddleButton ||
      (e->button() == Qt::LeftButton && (e->modifiers() & Qt::ControlModifier));
  if (startPan) {
    m_panning = true;
    m_lastPanPos = e->pos();
    viewport()->setCursor(Qt::ClosedHandCursor);
    e->accept();
    return;
  }
  QGraphicsView::mousePressEvent(e);
}

void VSRTLView::mouseMoveEvent(QMouseEvent *e) {
  if (m_panning) {
    const QPoint delta = e->pos() - m_lastPanPos;
    m_lastPanPos = e->pos();
    ensureNavigableSceneRect();
    horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
    verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
    e->accept();
    return;
  }
  QGraphicsView::mouseMoveEvent(e);
}

void VSRTLView::mouseReleaseEvent(QMouseEvent *e) {
  if (m_panning &&
      (e->button() == Qt::MiddleButton || e->button() == Qt::LeftButton)) {
    m_panning = false;
    viewport()->setCursor(Qt::ArrowCursor);
    e->accept();
    return;
  }
  QGraphicsView::mouseReleaseEvent(e);
}

void VSRTLView::resizeEvent(QResizeEvent *e) {
  QGraphicsView::resizeEvent(e);
  ensureNavigableSceneRect();
}
} // namespace vsrtl
