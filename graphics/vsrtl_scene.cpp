#include "vsrtl_scene.h"

#include <algorithm>
#include <iterator>

#include <QGraphicsSceneMouseEvent>

namespace vsrtl {

template <typename T>
T* getSingleSelectedItem(const QGraphicsScene& scene) {
    const auto selectedItems = scene.selectedItems();
    if (selectedItems.size() != 1) {
        return nullptr;
    }
    return dynamic_cast<T*>(selectedItems.at(0));
}

VSRTLScene::VSRTLScene(QObject* parent) : QGraphicsScene(parent) {
    connect(this, &QGraphicsScene::selectionChanged, this, &VSRTLScene::handleSelectionChanged);
}

/**
 * @brief VSRTLScene::handleWirePointMove
 *  For supporting drag/drop of points simultaneously with allowing point movement, we cannot use QDrag. Instead, the
 * scene will manage whether a single point is currently being dragged - if so, it will keep track of the WirePoint's
 * underneath the cursor, and notify them.
 * From this,we are then able to highlight the WirePoint's which are eligible for merging.
 */
void VSRTLScene::handleWirePointMove(QGraphicsSceneMouseEvent* event) {
    if (m_selectedPoint != nullptr && event->buttons() == Qt::LeftButton) {
        std::set<WirePoint*> pointsUnderCursor;
        for (const auto& item : items(event->scenePos())) {
            if (auto* point = dynamic_cast<WirePoint*>(item)) {
                pointsUnderCursor.insert(point);
            }
        }

        pointsUnderCursor.erase(m_selectedPoint);

        // notify any new points that they are now potential drop targets
        std::set<WirePoint*> diff;
        std::set_difference(pointsUnderCursor.begin(), pointsUnderCursor.end(), m_currentDropTargets.begin(),
                            m_currentDropTargets.end(), std::inserter(diff, diff.begin()));

        for (const auto& point : diff)
            point->pointDragEnter(m_selectedPoint);

        // Clear any old points which are no longer under the curser
        std::set<WirePoint*> oldPoints;
        std::set_difference(m_currentDropTargets.begin(), m_currentDropTargets.end(), pointsUnderCursor.begin(),
                            pointsUnderCursor.end(), std::inserter(oldPoints, oldPoints.begin()));

        for (const auto& oldPoint : oldPoints) {
            oldPoint->pointDragLeave(m_selectedPoint);
            m_currentDropTargets.erase(oldPoint);
        }

        // Add new points to the currently tracked drop targets
        m_currentDropTargets.insert(diff.begin(), diff.end());

    } else {
        for (const auto& oldTarget : m_currentDropTargets) {
            oldTarget->pointDragLeave(m_selectedPoint);
        }
        m_currentDropTargets.clear();
    }
}

void VSRTLScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    handleWirePointMove(event);

    return QGraphicsScene::mouseMoveEvent(event);
}

void VSRTLScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    if (m_selectedPoint && m_currentDropTargets.size() != 0) {
        Q_ASSERT(m_currentDropTargets.size() == 1);
        (*m_currentDropTargets.begin())->pointDrop(m_selectedPoint);

        for (const auto& point : m_currentDropTargets) {
            point->pointDragLeave(m_selectedPoint);
            m_currentDropTargets.erase(point);
        }
    }
    QGraphicsScene::mouseReleaseEvent(event);
}

void VSRTLScene::handleSelectionChanged() {
    m_selectedPoint = getSingleSelectedItem<WirePoint>(*this);
}

}  // namespace vsrtl
