#include "vsrtl_parameterdialog.h"
#include "ui_vsrtl_parameterdialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>

namespace vsrtl {

ParameterDialog::ParameterDialog(SimComponent *component, QWidget *parent)
    : QDialog(parent), m_ui(new Ui::ParameterDialog) {
  m_ui->setupUi(this);
  setWindowTitle("Parameters for: \"" +
                 QString::fromStdString(component->getDisplayName()) + "\"");

  for (const auto &p : component->getParameters()) {
    QLabel *label = new QLabel(QString::fromStdString(p->getName()));
    QWidget *editWidget = nullptr;
    if (auto *pt = dynamic_cast<Parameter<int> *>(p)) {
      auto *intWidget = new QSpinBox();
      intWidget->setValue(pt->getValue());
      editWidget = intWidget;

      const auto &options = pt->getOptions();
      if (options.size() == 2) {
        intWidget->setRange(options[0], options[1]);
      }

      connect(intWidget, QOverload<int>::of(&QSpinBox::valueChanged),
              [=](int value) {
                if (value != pt->getValue()) {
                  m_parameterChangeApplierFunctions[p] = [=] {
                    pt->setValue(value);
                  };
                } else {
                  m_parameterChangeApplierFunctions.erase(pt);
                }
              });
    } else if (auto *pt = dynamic_cast<Parameter<std::string> *>(p)) {
      auto *textWidget = new QLineEdit();
      textWidget->setText(QString::fromStdString(pt->getValue()));
      editWidget = textWidget;
      connect(textWidget, &QLineEdit::textChanged, [=](const QString &text) {
        if (text != QString::fromStdString(pt->getValue())) {
          m_parameterChangeApplierFunctions[p] = [=] {
            pt->setValue(text.toStdString());
          };
        } else {
          m_parameterChangeApplierFunctions.erase(pt);
        }
      });
    } else if (auto *pt = dynamic_cast<Parameter<bool> *>(p)) {
      auto *intWidget = new QCheckBox();
      intWidget->setChecked(pt->getValue());
      editWidget = intWidget;
      connect(intWidget, &QCheckBox::toggled, [=](bool state) {
        if (state != pt->getValue()) {
          m_parameterChangeApplierFunctions[p] = [=] { pt->setValue(state); };
        } else {
          m_parameterChangeApplierFunctions.erase(pt);
        }
      });
    } else if (auto *pt = dynamic_cast<Parameter<std::vector<int>> *>(p)) {
      Q_UNUSED(pt);
      Q_ASSERT(false && "todo");
    } else if (auto *pt =
                   dynamic_cast<Parameter<std::vector<std::string>> *>(p)) {
      Q_UNUSED(pt);
      Q_ASSERT(false && "todo");
    } else {
      Q_ASSERT(false && "Unknown parameter type");
    }

    label->setToolTip(QString::fromStdString(p->getTooltip()));
    editWidget->setToolTip(QString::fromStdString(p->getTooltip()));

    // Add widgets to layout
    const int row = m_ui->parameterLayout->rowCount();
    m_ui->parameterLayout->addWidget(label, row, 0, Qt::AlignLeft);
    m_ui->parameterLayout->addWidget(editWidget, row, 1, Qt::AlignRight);
  }

  resize(width(), height());
}

void ParameterDialog::accept() {
  for (const auto &f : m_parameterChangeApplierFunctions) {
    f.second();
  }
  QDialog::accept();
}

ParameterDialog::~ParameterDialog() { delete m_ui; }

} // namespace vsrtl
