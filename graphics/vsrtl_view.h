#ifndef VSRTL_VSRTLVIEW_H
#define VSRTL_VSRTLVIEW_H

#include <QGraphicsView>
#include "vsrtl_componentgraphic.h"

namespace vsrtl {

class VSRTLView : public QGraphicsView {
    Q_OBJECT
public:
    VSRTLView(QWidget* parent);
    ComponentGraphic* lookupGraphicForComponent(const Component* c);

protected:
    void wheelEvent(QWheelEvent*) override;

private slots:
    void setupMatrix();
    void zoomIn(int level = 1);
    void zoomOut(int level = 1);

private:
    qreal m_zoom;
};
}  // namespace vsrtl

#endif  // VSRTL_VSRTLVIEW_H
