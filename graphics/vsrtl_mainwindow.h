#ifndef VSRTL_MAINWINDOW_H
#define VSRTL_MAINWINDOW_H

#include <QMainWindow>
#include <memory>

QT_FORWARD_DECLARE_CLASS(QTreeView)

namespace vsrtl {

class VSRTLWidget;
class Architecture;
class NetlistModel;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(Architecture& arch, QWidget* parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow* ui;

    VSRTLWidget* m_vsrtlWidget;
    QTreeView* m_netlistView;
    NetlistModel* m_netlistModel;

    void createToolbar();
};

}  // namespace vsrtl

#endif  // VSRTL_MAINWINDOW_H
