#include "ripes_mainwindow.h"
#include "ui_ripes_mainwindow.h"

#include <QGraphicsScene>

namespace ripes {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    m_view = new RipesView(this);
    m_scene = new QGraphicsScene(this);
    m_view->setScene(m_scene);
    ui->viewLayout->addWidget(m_view);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::addComponent(ComponentGraphic* g) {
    m_scene->addItem(g);
}
}
