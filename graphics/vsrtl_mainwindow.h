#ifndef VSRTL_MAINWINDOW_H
#define VSRTL_MAINWINDOW_H

#include <QMainWindow>
#include <memory>

namespace vsrtl {

class VSRTLWidget;
class Architecture;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    void loadDesign(Architecture& arch);

private:
    Ui::MainWindow* ui;

    VSRTLWidget* m_vsrtlWidget;

    void createToolbar();
};

}  // namespace vsrtl

#endif  // VSRTL_MAINWINDOW_H
