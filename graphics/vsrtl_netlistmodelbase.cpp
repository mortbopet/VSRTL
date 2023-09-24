#include "vsrtl_netlistmodelbase.h"

namespace vsrtl {

int getRootIndex(QModelIndex index) {
  if (index.isValid()) {
    while (index.parent().isValid()) {
      index = index.parent();
    }
    return index.row();
  } else {
    return -1;
  }
}

int getRootSelectedIndex(QItemSelectionModel *model) {
  auto indexes = model->selectedIndexes();
  if (!indexes.isEmpty()) {
    return getRootIndex(indexes.first());
  } else {
    return -1;
  }
}

} // namespace vsrtl
