#ifndef VSRTL_NETLISTVIEW_H
#define VSRTL_NETLISTVIEW_H

#include <QMenu>
#include <QTreeView>

namespace vsrtl {

/**
 * Template parameter is expected to be a subclass of a TreeItem
 */
template <typename T>
class NetlistView : public QTreeView {
public:
  NetlistView(QWidget *parent) : QTreeView(parent) {
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QTreeView::customContextMenuRequested, this,
            &NetlistView<T>::contextMenuRequest);
  }

private slots:
  void contextMenuRequest(const QPoint &pos) {
    QModelIndex index = indexAt(pos);
    if (index.isValid()) {
      /* The TreeItem at the given index will be queried for its available
       * actions (both TreeItem actions and derived class actions). These
       * actions are then presented in the context menu */
      T *item = static_cast<T *>(index.internalPointer());

      QMenu menu;
      const auto &actions = item->getActions();
      if (actions.size() != 0) {
        for (const auto &actionMenu : actions) {
          menu.addSection(actionMenu->title());
          for (const auto &action : actionMenu->actions()) {
            menu.addAction(action);
          }
        }
        menu.exec(viewport()->mapToGlobal(pos));
      }
    }
  }
};

} // namespace vsrtl
#endif // VSRTL_NETLISTVIEW_H
