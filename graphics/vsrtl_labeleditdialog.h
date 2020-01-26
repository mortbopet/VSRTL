#pragma once

#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>

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

        connect(m_ui->text, &QLineEdit::textChanged, [=](const QString& text) {
            m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!text.isEmpty());
        });
    }
    ~LabelEditDialog() { delete m_ui; }
};

}  // namespace vsrtl
