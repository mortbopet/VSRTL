#ifndef VSRTL_WIDGET_H
#define VSRTL_WIDGET_H

#include <QMainWindow>
#include "vsrtl_circuithandler.h"
#include "vsrtl_componentgraphic.h"
#include "vsrtl_view.h"

QT_FORWARD_DECLARE_CLASS(QGraphicsScene)

namespace vsrtl {

namespace Ui {
class VSRTLWidget;
}

class Architecture;

class VSRTLWidget : public QWidget {
    Q_OBJECT

public:
    explicit VSRTLWidget(QWidget* parent = nullptr);
    ~VSRTLWidget();

    void addComponent(ComponentGraphic* g);
    void initializeDesign(Architecture* arch);

private:
    Ui::VSRTLWidget* ui;

    VSRTLView* m_view;
    QGraphicsScene* m_scene;

    CircuitHandler* m_ch;
};

}  // namespace vsrtl

#endif  // VSRTL_WIDGET_H