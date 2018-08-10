#ifndef RIPES_MAINWINDOW_H
#define RIPES_MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class Ripes_MainWindow;
}

class Ripes_MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit Ripes_MainWindow(QWidget* parent = 0);
    ~Ripes_MainWindow();

private:
    // Ui::Ripes_MainWindow *ui;
};

#endif  // RIPES_MAINWINDOW_H
