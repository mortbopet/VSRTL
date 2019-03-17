#include "vsrtl_treeitem.h"

#include <QMenu>

namespace vsrtl {

TreeItem::TreeItem(TreeItem* parent) {
    parentItem = parent;

    // Display type actions
    QActionGroup* displayTypeActionGroup = new QActionGroup(this);
    m_displayTypeMenu = new QMenu("Display type");

    QAction* hexTypeAction = new QAction("Hex", this);
    displayTypeActionGroup->addAction(hexTypeAction);
    hexTypeAction->setCheckable(true);
    connect(hexTypeAction, &QAction::triggered, [=](bool checked) {
        if (checked)
            m_displayType = DisplayType::Hex;
    });
    m_displayTypeMenu->addAction(hexTypeAction);

    QAction* binTypeAction = new QAction("Binary", this);
    displayTypeActionGroup->addAction(binTypeAction);
    binTypeAction->setCheckable(true);
    connect(binTypeAction, &QAction::triggered, [=](bool checked) {
        if (checked)
            m_displayType = DisplayType::Binary;
    });
    m_displayTypeMenu->addAction(binTypeAction);

    QAction* unsignedTypeAction = new QAction("Unsigned", this);
    displayTypeActionGroup->addAction(unsignedTypeAction);
    unsignedTypeAction->setCheckable(true);
    connect(unsignedTypeAction, &QAction::triggered, [=](bool checked) {
        if (checked)
            m_displayType = DisplayType::Unsigned;
    });
    m_displayTypeMenu->addAction(unsignedTypeAction);

    QAction* signedTypeAction = new QAction("Signed", this);
    displayTypeActionGroup->addAction(signedTypeAction);
    signedTypeAction->setCheckable(true);
    connect(signedTypeAction, &QAction::triggered, [=](bool checked) {
        if (checked)
            m_displayType = DisplayType::Signed;
    });
    m_displayTypeMenu->addAction(signedTypeAction);

    displayTypeActionGroup->setExclusive(true);
    hexTypeAction->setChecked(true);
}

TreeItem::~TreeItem() {
    qDeleteAll(childItems);
}

TreeItem* TreeItem::child(int number) {
    return childItems.value(number);
}

int TreeItem::childCount() const {
    return childItems.count();
}

int TreeItem::childNumber() const {
    if (parentItem)
        return parentItem->childItems.indexOf(const_cast<TreeItem*>(this));

    return 0;
}

QList<QMenu*> TreeItem::getActions() const {
    return {m_displayTypeMenu};
}

bool TreeItem::insertChild(int position, TreeItem* item) {
    if (position < 0 || position > childItems.size())
        return false;

    childItems.insert(position, item);

    return true;
}

TreeItem* TreeItem::parent() {
    return parentItem;
}

bool TreeItem::removeChildren(int position, int count) {
    if (position < 0 || position + count > childItems.size())
        return false;

    for (int row = 0; row < count; ++row)
        delete childItems.takeAt(position);

    return true;
}

}  // namespace vsrtl
