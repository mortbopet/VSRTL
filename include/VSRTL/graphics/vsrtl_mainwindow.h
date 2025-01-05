#ifndef VSRTL_MAINWINDOW_H
#define VSRTL_MAINWINDOW_H

#include <QMainWindow>
#include <memory>

#include "../interface/vsrtl_interface.h"

QT_FORWARD_DECLARE_CLASS(QTreeView)

namespace vsrtl {

class VSRTLWidget;
class NetlistModel;
class Netlist;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(SimDesign &arch, QWidget *parent = nullptr);
  ~MainWindow();

private:
  Ui::MainWindow *ui;

  VSRTLWidget *m_vsrtlWidget;
  Netlist *m_netlist;

  void createToolbar();
};

} // namespace vsrtl

#endif // VSRTL_MAINWINDOW_H
