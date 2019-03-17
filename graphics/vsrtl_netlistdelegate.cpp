#include "vsrtl_netlistdelegate.h"
#include "vsrtl_defines.h"
#include "vsrtl_displaytype.h"
#include "vsrtl_treeitem.h"

#include "vsrtl_registermodel.h"

#include <QApplication>
#include <QLineEdit>

namespace vsrtl {

NetlistDelegate::NetlistDelegate(QObject* parent) : QStyledItemDelegate(parent) {
    // The validator does not concern itself whether the value is actually writeable to the register. Any hex input is
    // accepted. When a register is forced to a value, the value is truncated to the bit width of the register.
    m_validator = new QRegExpValidator(this);
}

QWidget* NetlistDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                                       const QModelIndex& index) const {
    QLineEdit* editor = new QLineEdit(parent);
    editor->setFont(QFont("monospace"));

    QPalette palette = QApplication::palette();
    palette.setBrush(QPalette::Text, Qt::blue);
    editor->setPalette(palette);
    editor->setValidator(m_validator);

    return editor;
}

void NetlistDelegate::setEditorData(QWidget* e, const QModelIndex& index) const {
    auto* treeItem = static_cast<TreeItem*>(index.internalPointer());

    switch (treeItem->m_displayType) {
        case DisplayType::Binary: {
            m_validator->setRegExp(binRegex);
            break;
        }
        case DisplayType::Hex: {
            m_validator->setRegExp(hexRegex);
            break;
        }
        case DisplayType::Unsigned: {
            m_validator->setRegExp(unsignedRegex);
            break;
        }
        case DisplayType::Signed: {
            m_validator->setRegExp(signedRegex);
            break;
        }
    }

    QLineEdit* editor = dynamic_cast<QLineEdit*>(e);
    if (editor) {
        editor->setText(index.data().toString());
    }
}

void NetlistDelegate::setModelData(QWidget* e, QAbstractItemModel* model, const QModelIndex& index) const {
    QLineEdit* editor = dynamic_cast<QLineEdit*>(e);
    if (editor) {
        model->setData(index, editor->text(), Qt::EditRole);
    }
}

}  // namespace vsrtl
