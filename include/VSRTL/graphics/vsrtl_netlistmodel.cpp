#include <QItemSelectionModel>
#include <QWidget>

#include <QDebug>
#include <QIcon>

#include "vsrtl_netlistmodel.h"

#include "../interface/vsrtl_gfxobjecttypes.h"
#include "../interface/vsrtl_interface.h"

namespace vsrtl {

NetlistModel::NetlistModel(SimDesign *arch, QObject *parent)
    : NetlistModelBase({"Component", "I/O", "Value", "Width"}, arch, parent) {
  rootItem = new NetlistTreeItem(nullptr);

  loadDesign(rootItem, m_arch);
}

QVariant NetlistModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid())
    return QVariant();

  NetlistTreeItem *item = getTreeItem(index);
  return item->data(index.column(), role);
}

Qt::ItemFlags NetlistModel::flags(const QModelIndex &index) const {
  if (!index.isValid())
    return Qt::ItemFlags();
  Qt::ItemFlags flags = QAbstractItemModel::flags(index);

  // Output ports of register components are editable.
  // Check if parent component is a Register, and if the current port is an
  // output port. If so, the port is editable
  if (indexIsRegisterOutputPortValue(index)) {
    flags |= Qt::ItemIsEditable;
  }

  return flags;
}

bool NetlistModel::setData(const QModelIndex &index, const QVariant &value,
                           int) {
  if (indexIsRegisterOutputPortValue(index)) {
    SimSynchronous *reg =
        dynamic_cast<SimSynchronous *>(getParentComponent(index));
    if (reg) {
      reg->forceValue(0, value.toInt());
      assert(false && "Todo: No way to propagate design through interface");
      // m_arch.propagateDesign();
      return true;
    }
  }
  return false;
}

void NetlistModel::invalidate() {
  // Data changed within Design, invalidate value column
  emit dataChanged(index(0, ValueColumn), index(rowCount(), ValueColumn),
                   {Qt::DisplayRole});
}

QModelIndex NetlistModel::lookupIndexForComponent(SimComponent *c) const {
  if (m_componentIndicies.find(c) != m_componentIndicies.end()) {
    NetlistTreeItem *item = m_componentIndicies.at(c);
    if (item->index.isValid()) {
      return item->index;
    }
  }
  return QModelIndex();
}

void NetlistModel::addPortToComponent(SimPort *port, NetlistTreeItem *parent,
                                      PortDirection dir) {
  auto *child = new NetlistTreeItem(parent);
  child->setPort(port);
  parent->insertChild(parent->childCount(), child);
  child->m_direction = dir;
}

bool NetlistModel::indexIsRegisterOutputPortValue(
    const QModelIndex &index) const {
  if (index.column() == ValueColumn) {
    auto *item = getTreeItem(index);
    auto *parentItem = static_cast<NetlistTreeItem *>(item->parent());
    auto *parentComponent = parentItem->m_component;
    if (parentComponent) {
      if (dynamic_cast<SimSynchronous *>(parentComponent)) {
        // Parent is register, check if current index is an output port
        if (item->m_port && item->m_direction == PortDirection::Output) {
          return true;
        }
      }
    }
  }
  return false;
}

void NetlistModel::loadDesignRecursive(NetlistTreeItem *parent,
                                       SimComponent *component) {
  // Subcomponents
  for (const auto &subcomponent : component->getSubComponents()) {
    if (subcomponent->getGraphicsType() == GraphicsTypeFor(Constant)) {
      // Do not display constants in the netlist
      continue;
    }

    auto *child = new NetlistTreeItem(parent);
    parent->insertChild(parent->childCount(), child);

    // Set component data (component pointer and name)
    m_componentIndicies[subcomponent] = child;

    child->m_component = subcomponent;
    child->m_name = QString::fromStdString(subcomponent->getName());

    // Recurse into the child
    loadDesignRecursive(child, subcomponent);
  }

  // I/O ports of component
  for (const auto &input : component->getPorts<SimPort::PortType::in>()) {
    addPortToComponent(input, parent, PortDirection::Input);
  }
  for (const auto &output : component->getPorts<SimPort::PortType::out>()) {
    addPortToComponent(output, parent, PortDirection::Output);
  }
}

SimComponent *NetlistModel::getParentComponent(const QModelIndex &index) const {
  auto *item = getTreeItem(index);
  if (item) {
    item = static_cast<NetlistTreeItem *>(item->parent());
    if (item) {
      return item->m_component;
    }
  }
  return nullptr;
}

void NetlistModel::loadDesign(NetlistTreeItem *parent, SimDesign *design) {
  loadDesignRecursive(parent, design);
}
} // namespace vsrtl
