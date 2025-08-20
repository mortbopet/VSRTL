#pragma once

#include <QButtonGroup>
#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>

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
  explicit LabelEditDialog(QWidget *parent = nullptr);

  void setAlignment(Qt::Alignment alignment);

  Qt::Alignment getAlignment() const;

  ~LabelEditDialog();
};

} // namespace vsrtl
