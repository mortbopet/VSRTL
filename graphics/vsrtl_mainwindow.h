#ifndef VSRTL_MAINWINDOW_H
#define VSRTL_MAINWINDOW_H

#include <QMainWindow>
#include "vsrtl_circuithandler.h"
#include "vsrtl_componentgraphic.h"
#include "vsrtl_view.h"

QT_FORWARD_DECLARE_CLASS(QGraphicsScene)

namespace vsrtl {

namespace Ui {
class MainWindow;
}
class Architecture;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = 0);
    ~MainWindow();

    void addComponent(ComponentGraphic* g);
    void initializeArchitecture(Architecture* arch);

private:
    Ui::MainWindow* ui;

    VSRTLView* m_view;
    QGraphicsScene* m_scene;

    CircuitHandler* m_ch;
};
}

#endif  // VSRTL_MAINWINDOW_H
