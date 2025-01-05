#pragma once

#include <QDialog>

#include <functional>
#include <map>

#include "vsrtl_interface.h"

namespace vsrtl {

class ParameterBase;

namespace Ui {
class ParameterDialog;
}

class ParameterDialog : public QDialog {
  Q_OBJECT

public:
  ParameterDialog(SimComponent *component, QWidget *parent = nullptr);
  ~ParameterDialog();

  void accept() override;

private:
  Ui::ParameterDialog *m_ui;

  std::map<ParameterBase *, std::function<void()>>
      m_parameterChangeApplierFunctions;
};

} // namespace vsrtl
