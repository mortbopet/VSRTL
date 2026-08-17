#ifndef VSRTL_VSRTLVIEW_H
#define VSRTL_VSRTLVIEW_H

#include "VSRTL/graphics/vsrtl_componentgraphic.h"
#include <QGraphicsView>
#include <QOpenGLWidget>

namespace vsrtl {

class VSRTLView : public QGraphicsView {
  Q_OBJECT

  static constexpr double s_zoomDefault = 250.0;
  static constexpr double s_zoomScale = 50.0;
  static constexpr double s_zoomInterval = 6.0;

public:
  VSRTLView(QWidget *parent);
  ComponentGraphic *lookupGraphicForComponent(const SimComponent *c);

  /**
   * @brief zoomToFit
   * Adjusts level of zoom to ensure that @p item is visible
   */
  void zoomToFit(const QGraphicsItem *item);

protected:
  void wheelEvent(QWheelEvent *) override;
  void mousePressEvent(QMouseEvent *) override;
  void mouseMoveEvent(QMouseEvent *) override;
  void mouseReleaseEvent(QMouseEvent *) override;
  void resizeEvent(QResizeEvent *) override;

private slots:
  void setupMatrix();
  void zoomIn(double level = s_zoomInterval);
  void zoomOut(double level = s_zoomInterval);

private:
  /**
   * @brief ensureNavigableSceneRect
   * Grows the scene rect so that it always extends generously beyond both the
   * content and the current viewport. This makes the view freely pannable in
   * every direction and allows the content to be centered regardless of its
   * aspect ratio. The current view center is preserved across the change.
   */
  void ensureNavigableSceneRect();

  QOpenGLWidget *m_renderer;
  double m_zoom = s_zoomDefault;
  bool m_panning = false;
  QPoint m_lastPanPos;
};
} // namespace vsrtl

#endif // VSRTL_VSRTLVIEW_H
