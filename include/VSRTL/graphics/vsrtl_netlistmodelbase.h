#ifndef VSRTL_NETLISTMODELBASE_H
#define VSRTL_NETLISTMODELBASE_H

#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <QModelIndex>
#include <QVariant>

#include "../interface/vsrtl_interface.h"

namespace vsrtl {

int getRootIndex(QModelIndex index);
int getRootSelectedIndex(QItemSelectionModel *model);

template <typename T>
class NetlistModelBase : public QAbstractItemModel {
public:
  NetlistModelBase(QStringList headers, SimDesign *arch,
                   QObject *parent = nullptr)
      : QAbstractItemModel(parent), m_headers(headers), m_arch(arch) {}

  ~NetlistModelBase() override { delete rootItem; }

  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const override {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole &&
        m_headers.size() > section)
      return m_headers[section];

    return QVariant();
  }

  QModelIndex index(int row, int column,
                    const QModelIndex &parent = QModelIndex()) const override {
    if (parent.isValid() && parent.column() != 0)
      return QModelIndex();

    T *parentItem = getTreeItem(parent);
    if (!parentItem)
      parentItem = rootItem;

    Q_ASSERT(rootItem);
    T *childItem = static_cast<T *>(parentItem->child(row));
    if (childItem) {
      auto i = createIndex(row, column, childItem);
      childItem->index = i;
      return i;
    } else
      return QModelIndex();
  }

  QModelIndex parent(const QModelIndex &index) const override {
    if (!index.isValid())
      return QModelIndex();

    auto *childItem = getTreeItem(index);
    auto *parentItem = childItem->parent();

    Q_ASSERT(rootItem);
    if (parentItem == rootItem)
      return QModelIndex();

    return createIndex(parentItem->childNumber(), 0, parentItem);
  }

  int rowCount(const QModelIndex &parent = QModelIndex()) const override {
    auto *parentItem = getTreeItem(parent);
    return parentItem->childCount();
  }

  int columnCount(const QModelIndex & = QModelIndex()) const override {
    return m_headers.size();
  }

  bool setHeaderData(int section, Qt::Orientation orientation,
                     const QVariant &value, int role = Qt::EditRole) override {
    if (role != Qt::EditRole || orientation != Qt::Horizontal ||
        section > m_headers.size())
      return false;
    m_headers[section] = value.toString();

    emit headerDataChanged(orientation, section, section);
    return true;
  }
public slots:
  virtual void invalidate() = 0;

protected:
  T *getTreeItem(const QModelIndex &index) const {
    if (index.isValid()) {
      return static_cast<T *>(index.internalPointer());
    }
    return rootItem;
  }

  T *rootItem = nullptr;
  QStringList m_headers;
  SimDesign *m_arch = nullptr;
};

} // namespace vsrtl

#endif // VSRTL_NETLISTMODELBASE_H
