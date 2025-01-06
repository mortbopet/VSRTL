#ifndef VSRTL_NETLISTDELEGATE_H
#define VSRTL_NETLISTDELEGATE_H

#include <QRegularExpressionValidator>
#include <QStyledItemDelegate>

namespace vsrtl {

class NetlistDelegate : public QStyledItemDelegate {
public:
  NetlistDelegate(QObject *parent);
  QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                        const QModelIndex &index) const override;
  void setEditorData(QWidget *editor, const QModelIndex &index) const override;
  void setModelData(QWidget *editor, QAbstractItemModel *model,
                    const QModelIndex &index) const override;

private:
  QRegularExpressionValidator *m_validator;
};
} // namespace vsrtl

#endif // VSRTL_NETLISTDELEGATE_H
