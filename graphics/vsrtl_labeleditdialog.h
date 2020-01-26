#pragma once

#include <QDialog>

#include "ui_vsrtl_labeleditdialog.h"

namespace vsrtl {

namespace Ui {
class LabelEditDialog;
}

class LabelEditDialog : public QDialog {
    Q_OBJECT
    friend class Label;

private:
    Ui::LabelEditDialog* m_ui;

public:
    explicit LabelEditDialog(QWidget* parent = nullptr) : QDialog(parent), m_ui(new Ui::LabelEditDialog) {
        m_ui->setupUi(this);
        setWindowTitle("Edit label");
    }
    ~LabelEditDialog() { delete m_ui; }
};

}  // namespace vsrtl
