#include "vsrtl_treeitem.h"
#include "vsrtl_netlistmodel.h"

#include <QMenu>

namespace vsrtl {

TreeItem::TreeItem(TreeItem *parent) { parentItem = parent; }

TreeItem::~TreeItem() { qDeleteAll(childItems); }

TreeItem *TreeItem::child(int number) { return childItems.value(number); }

int TreeItem::childCount() const { return childItems.count(); }

int TreeItem::childNumber() const {
  if (parentItem)
    return parentItem->childItems.indexOf(const_cast<TreeItem *>(this));

  return 0;
}

bool TreeItem::insertChild(int position, TreeItem *item) {
  if (position < 0 || position > childItems.size())
    return false;

  childItems.insert(position, item);

  return true;
}

TreeItem *TreeItem::parent() { return parentItem; }

bool TreeItem::removeChildren(int position, int count) {
  if (position < 0 || position + count > childItems.size())
    return false;

  for (int row = 0; row < count; ++row)
    delete childItems.takeAt(position);

  return true;
}

NetlistTreeItem::NetlistTreeItem(TreeItem *parent) : TreeItem(parent) {}

void NetlistTreeItem::setPort(SimPort *port) {
  Q_ASSERT(port);
  m_port = port;
  m_name = QString::fromStdString(m_port->getName());
  m_radixMenu = createPortRadixMenu(m_port, m_radix);

  if (m_port->isEnumPort()) {
    m_radix = Radix::Enum;
  }
}

QList<QMenu *> NetlistTreeItem::getActions() const {
  // Only return actions for items which have a port (default actions from
  // TreeItem displays display type actions, which are not applicable for
  // Component items)
  if (m_radixMenu) {
    return {m_radixMenu};
  }
  return QList<QMenu *>();
}

QVariant NetlistTreeItem::data(int column, int role) const {
  if (column == NetlistModel::IOColumn && role == Qt::DecorationRole &&
      m_port != nullptr) {
    return m_direction == PortDirection::Input
               ? QIcon(":/vsrtl_icons/input.svg")
               : QIcon(":/vsrtl_icons/output.svg");
  } else if (role == Qt::DisplayRole || role == Qt::EditRole) {
    switch (column) {
    case NetlistModel::ComponentColumn: {
      return m_name;
    }
    case NetlistModel::ValueColumn: {
      if (m_port) {
        return encodePortRadixValue(m_port, m_radix);
      }
      break;
    }
    case NetlistModel::WidthColumn: {
      if (m_port) {
        return m_port->getWidth();
      }
      break;
    }
    }
  }
  return QVariant();
}
bool NetlistTreeItem::setData(int, const QVariant &, int) { return false; }

} // namespace vsrtl
