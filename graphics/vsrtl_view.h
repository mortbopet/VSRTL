#ifndef VSRTL_VSRTLVIEW_H
#define VSRTL_VSRTLVIEW_H

#include "vsrtl_componentgraphic.h"
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

private slots:
  void setupMatrix();
  void zoomIn(double level = s_zoomInterval);
  void zoomOut(double level = s_zoomInterval);

private:
  QOpenGLWidget *m_renderer;
  double m_zoom = s_zoomDefault;
};
} // namespace vsrtl

#endif // VSRTL_VSRTLVIEW_H
