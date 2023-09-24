#pragma once

#include <QButtonGroup>
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
  Ui::LabelEditDialog *m_ui = nullptr;

public:
  explicit LabelEditDialog(QWidget *parent = nullptr)
      : QDialog(parent), m_ui(new Ui::LabelEditDialog) {
    m_ui->setupUi(this);
    setWindowTitle("Edit label");

    m_ui->alignLeft->setIcon(QIcon(":/vsrtl_icons/alignleft.svg"));
    m_ui->alignCenter->setIcon(QIcon(":/vsrtl_icons/aligncenter.svg"));
    m_ui->alignRight->setIcon(QIcon(":/vsrtl_icons/alignright.svg"));

    QButtonGroup *alignment = new QButtonGroup(this);
    alignment->addButton(m_ui->alignLeft);
    alignment->addButton(m_ui->alignCenter);
    alignment->addButton(m_ui->alignRight);
    alignment->setExclusive(true);
    m_ui->alignCenter->setChecked(true);

    connect(m_ui->text, &QTextEdit::textChanged, [=]() {
      const auto text = m_ui->text->toPlainText();
      m_ui->buttonBox->button(QDialogButtonBox::Ok)
          ->setEnabled(!text.isEmpty());
    });
  }

  void setAlignment(Qt::Alignment alignment) {
    switch (alignment) {
    case Qt::AlignLeft: {
      m_ui->alignLeft->setChecked(true);
      break;
    }
    case Qt::AlignCenter: {
      m_ui->alignCenter->setChecked(true);
      break;
    }
    case Qt::AlignRight: {
      m_ui->alignRight->setChecked(true);
      break;
    }
    }
  }

  Qt::Alignment getAlignment() const {
    if (m_ui->alignLeft->isChecked()) {
      return Qt::AlignLeft;
    } else if (m_ui->alignCenter->isChecked()) {
      return Qt::AlignCenter;
    } else if (m_ui->alignRight->isChecked()) {
      return Qt::AlignRight;
    }
    Q_UNREACHABLE();
  }

  ~LabelEditDialog() { delete m_ui; }
};

} // namespace vsrtl
