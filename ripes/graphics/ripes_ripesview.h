#ifndef RIPES_RIPESVIEW_H
#define RIPES_RIPESVIEW_H

#include <QGraphicsView>

namespace ripes {

class RipesView : public QGraphicsView {
    Q_OBJECT
public:
    RipesView(QWidget* parent);

protected:
    void wheelEvent(QWheelEvent*) override;

private slots:
    void setupMatrix();
    void zoomIn(int level = 1);
    void zoomOut(int level = 1);

private:
    qreal m_zoom;
};
}

#endif  // RIPES_RIPESVIEW_H
