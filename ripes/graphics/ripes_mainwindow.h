#ifndef RIPES_MAINWINDOW_H
#define RIPES_MAINWINDOW_H

#include <QMainWindow>
#include "ripes_componentgraphic.h"
#include "ripes_ripesview.h"

QT_FORWARD_DECLARE_CLASS(QGraphicsScene)

namespace ripes {

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = 0);
    ~MainWindow();

    void addComponent(ComponentGraphic* g);

private:
    Ui::MainWindow* ui;

    RipesView* m_view;
    QGraphicsScene* m_scene;
};
}

#endif  // RIPES_MAINWINDOW_H
