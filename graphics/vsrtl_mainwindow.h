#ifndef VSRTL_MAINWINDOW_H
#define VSRTL_MAINWINDOW_H

#include <QMainWindow>

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

    void loadDesign(Architecture* arch);

private:
    Ui::MainWindow* ui;

    VSRTLWidget* m_vsrtlWidget;
};

}  // namespace vsrtl

#endif  // VSRTL_MAINWINDOW_H
