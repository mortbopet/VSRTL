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
  if (tlc == nullptr || scene() == nullptr) {
    return;
  }

  // Fit the *visible* scene content, with a small margin so nothing is clipped
  // at the viewport edges. We compute this from visible items only: hidden wire
  // graphics retain large, stale bounding rects, so relying on the top-level
  // component's childrenBoundingRect (or the scene's itemsBoundingRect) would
  // inflate the fitted area and push the diagram off-center.
  QRectF target;
  const auto sceneItems = scene()->items();
  for (auto *item : sceneItems) {
    if (item->isVisibleTo(nullptr))
      target = target.united(item->sceneBoundingRect());
  }
  if (target.isNull())
    target = tlc->sceneBoundingRect();
  const qreal margin = 0.02 * std::max(target.width(), target.height());
  target.adjust(-margin, -margin, margin, margin);

  // Ensure there is room around the content in every direction *before*
  // fitting, so fitInView can properly center the target within the viewport.
  ensureNavigableSceneRect();

  // fitInView both scales and centers the target within the viewport.
  fitInView(target, Qt::KeepAspectRatio);
  const qreal scale = transform().m11();

  if (scale < 0.1) {
    // There is some strange bug in the QGraphicsScene/QGraphicsView logic which
    // does not calculate the bounding matrix of the top level component
    // correctly if the view is not currently visible. In this case, revert to
    // the default zoom amount.
    m_zoom = s_zoomDefault;
    setupMatrix();
  } else {
    // Keep the zoom state in sync with the scale that fitInView applied,
    // without re-applying the transform via setupMatrix() (which would reset
    // the view to the top-left and undo fitInView's centering).
    m_zoom = std::log2(scale) * s_zoomScale + s_zoomDefault;
  }
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
