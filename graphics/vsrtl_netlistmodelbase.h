#ifndef VSRTL_NETLISTMODELBASE_H
#define VSRTL_NETLISTMODELBASE_H

#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <QModelIndex>
#include <QVariant>

namespace vsrtl {

class Design;
class Component;
class Port;

int getRootIndex(QModelIndex index);
int getRootSelectedIndex(QItemSelectionModel* model);

template <typename T>
class NetlistModelBase : public QAbstractItemModel {
public:
    NetlistModelBase(QStringList headers, const Design& arch, QObject* parent = nullptr)
        : m_headers(headers), m_arch(arch), QAbstractItemModel(parent) {}

    ~NetlistModelBase() override { delete rootItem; }

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole && m_headers.size() > section)
            return m_headers[section];

        return QVariant();
    }

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override {
        if (parent.isValid() && parent.column() != 0)
            return QModelIndex();

        T* parentItem = getItem(parent);
        if (!parentItem)
            parentItem = rootItem;

        T* childItem = static_cast<T*>(parentItem->child(row));
        if (childItem) {
            auto i = createIndex(row, column, childItem);
            childItem->index = i;
            return i;
        } else
            return QModelIndex();
    }

    QModelIndex parent(const QModelIndex& index) const override {
        if (!index.isValid())
            return QModelIndex();

        auto* childItem = getItem(index);
        auto* parentItem = childItem->parent();

        if (parentItem == rootItem)
            return QModelIndex();

        return createIndex(parentItem->childNumber(), 0, parentItem);
    }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override {
        auto* parentItem = getItem(parent);
        return parentItem->childCount();
    }

    int columnCount(const QModelIndex& = QModelIndex()) const override { return m_headers.size(); }

    bool setHeaderData(int section, Qt::Orientation orientation, const QVariant& value,
                       int role = Qt::EditRole) override {
        if (role != Qt::EditRole || orientation != Qt::Horizontal || section > m_headers.size())
            return false;
        m_headers[section] = value.toString();

        emit headerDataChanged(orientation, section, section);
        return true;
    }
public slots:
    void invalidate() { dataChanged(index(0, 0), index(rowCount(), columnCount())); }

protected:
    T* getItem(const QModelIndex& index) const {
        if (index.isValid()) {
            return static_cast<T*>(index.internalPointer());
        }
        return rootItem;
    }

    T* rootItem = nullptr;
    const Design& m_arch;
    QStringList m_headers;
};

}  // namespace vsrtl

#endif  // VSRTL_NETLISTMODELBASE_H
