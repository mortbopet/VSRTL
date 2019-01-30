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

class Design;

class VSRTLWidget : public QWidget {
    Q_OBJECT

public:
    explicit VSRTLWidget(Design& arch, QWidget* parent = nullptr);
    ~VSRTLWidget();

    void addComponent(ComponentGraphic* g);

public slots:
    void clock();
    void reset();

private:
    void initializeDesign(Design& arch);

    Ui::VSRTLWidget* ui;

    VSRTLView* m_view;
    QGraphicsScene* m_scene;

    CircuitHandler* m_ch;
    Design& m_arch;
};

}  // namespace vsrtl

#endif  // VSRTL_WIDGET_H
