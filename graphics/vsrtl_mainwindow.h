#ifndef VSRTL_MAINWINDOW_H
#define VSRTL_MAINWINDOW_H

#include <QMainWindow>
#include <memory>

QT_FORWARD_DECLARE_CLASS(QTreeView)

namespace vsrtl {

class VSRTLWidget;
class NetlistModel;
class Netlist;

namespace Ui {
class MainWindow;
}

namespace core {
class Design;
}
using namespace core;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(Design& arch, QWidget* parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow* ui;

    VSRTLWidget* m_vsrtlWidget;
    Netlist* m_netlist;

    void createToolbar();
};

}  // namespace vsrtl

#endif  // VSRTL_MAINWINDOW_H
