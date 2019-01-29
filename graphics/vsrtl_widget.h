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
    explicit VSRTLWidget(Architecture& arch, QWidget* parent = nullptr);
    ~VSRTLWidget();

    void addComponent(ComponentGraphic* g);

public slots:
    void clock();
    void reset();

private:
    void initializeDesign(Architecture& arch);

    Ui::VSRTLWidget* ui;

    VSRTLView* m_view;
    QGraphicsScene* m_scene;

    CircuitHandler* m_ch;
    Architecture& m_arch;
};

}  // namespace vsrtl

#endif  // VSRTL_WIDGET_H
