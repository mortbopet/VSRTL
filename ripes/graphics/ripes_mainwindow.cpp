#include "ripes_mainwindow.h"
#include "ui_ripes_mainwindow.h"

#include <QGraphicsScene>

namespace ripes {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    m_scene = new QGraphicsScene(this);
    ui->view->setScene(m_scene);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::addComponent(ComponentGraphic* g) {
    m_scene->addItem(g);
}
}
