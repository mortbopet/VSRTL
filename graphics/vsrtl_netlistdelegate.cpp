#include "vsrtl_netlistdelegate.h"
#include "vsrtl_defines.h"

#include <QApplication>
#include <QLineEdit>

namespace vsrtl {

NetlistDelegate::NetlistDelegate(QObject* parent) : QStyledItemDelegate(parent) {
    // The validator does not concern itself whether the value is actually writeable to the register. Any hex input is
    // accepted. When a register is forced to a value, the value is truncated to the bit width of the register.
    m_validator = new QRegExpValidator(this);
    m_validator->setRegExp(QRegExp("0[xX][0-9a-fA-F]+"));
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
    QLineEdit* editor = dynamic_cast<QLineEdit*>(e);
    if (editor) {
        editor->setText(index.data().toString());
    }
}
void NetlistDelegate::setModelData(QWidget* e, QAbstractItemModel* model, const QModelIndex& index) const {
    QLineEdit* editor = dynamic_cast<QLineEdit*>(e);
    if (editor) {
        bool ok = false;
        VSRTL_VT_U value = editor->text().toULong(&ok, 16);
        if (ok) {
            model->setData(index, value, Qt::EditRole);
        }
    }
}

}  // namespace vsrtl
