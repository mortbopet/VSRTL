#include "VSRTL/graphics/vsrtl_netlistdelegate.h"
#include "VSRTL/graphics/vsrtl_netlistmodel.h"
#include "VSRTL/graphics/vsrtl_radix.h"
#include "VSRTL/interface/vsrtl_defines.h"

#include "VSRTL/graphics/vsrtl_registermodel.h"

#include <QApplication>
#include <QLineEdit>

namespace vsrtl {

NetlistDelegate::NetlistDelegate(QObject *parent)
    : QStyledItemDelegate(parent) {
  // The validator does not concern itself whether the value is actually
  // writeable to the register. Any hex input is accepted. When a register is
  // forced to a value, the value is truncated to the bit width of the register.
  m_validator = new QRegularExpressionValidator(this);
}

QWidget *NetlistDelegate::createEditor(QWidget *parent,
                                       const QStyleOptionViewItem &,
                                       const QModelIndex &) const {
  QLineEdit *editor = new QLineEdit(parent);
  editor->setFont(QFont("monospace"));

  QPalette palette = QApplication::palette();
  // Use a blue that stays legible on both light and dark backgrounds.
  const bool dark = palette.color(QPalette::Base).lightness() < 128;
  palette.setBrush(QPalette::Text, dark ? QColor(0x4d, 0x9e, 0xff) : Qt::blue);
  editor->setPalette(palette);
  editor->setValidator(m_validator);

  return editor;
}

void NetlistDelegate::setEditorData(QWidget *e,
                                    const QModelIndex &index) const {
  auto *treeItem = static_cast<NetlistTreeItem *>(index.internalPointer());

  switch (treeItem->m_radix) {
  case Radix::Binary: {
    m_validator->setRegularExpression(binRegex);
    break;
  }
  case Radix::Hex: {
    m_validator->setRegularExpression(hexRegex);
    break;
  }
  case Radix::Unsigned: {
    m_validator->setRegularExpression(unsignedRegex);
    break;
  }
  case Radix::Signed: {
    m_validator->setRegularExpression(signedRegex);
    break;
  }
  case Radix::Enum: {
    return;
  }
  }

  QLineEdit *editor = dynamic_cast<QLineEdit *>(e);
  if (editor) {
    editor->setText(index.data().toString());
  }
}

void NetlistDelegate::setModelData(QWidget *e, QAbstractItemModel *model,
                                   const QModelIndex &index) const {
  QLineEdit *editor = dynamic_cast<QLineEdit *>(e);
  if (editor) {
    model->setData(index, editor->text(), Qt::EditRole);
  }
}

} // namespace vsrtl
