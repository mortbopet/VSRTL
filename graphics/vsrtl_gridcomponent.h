#pragma once

#include <QRect>
#include <map>

#include "../interface/vsrtl_interface.h"
#include "vsrtl_graphicsbase.h"
#include "vsrtl_shape.h"

namespace vsrtl {

enum class CSys { Local, Parent, Global, Scene };

/** class CPoint
 * Typesafe QPoint based on coordinate system
 */
template <CSys c>
class CPoint : private QPoint {
public:
    CPoint(QPoint p) : QPoint(p) {}
    CPoint() : QPoint() {}

    // QPoint of this point may only explicitely be accessed through get()
    QPoint& get() { return *this; }
};

enum class Side { Left, Right, Top, Bottom };
struct PortPos {
    Side dir;
    int index;
    bool validIndex() const { return index != -1; }
    bool uninitialized() const { return index < 0; }
};

class ComponentBorder {
public:
    using IdToPortMap = std::map<int, const SimPort*>;
    using PortToIdMap = std::map<const SimPort*, int>;

    ComponentBorder(const SimComponent& c) {
        // Input- and outputs are initialized to uninitialized (<0) indicies on the left- and right side
        for (const auto& p : c.getPorts<SimPort::Direction::in>()) {
            m_portMap[p] = nullptr;
            addPortToSide(PortPos{Side::Left, int(-(m_left.count() + 1))}, p);
        }
        for (const auto& p : c.getPorts<SimPort::Direction::out>()) {
            m_portMap[p] = nullptr;
            addPortToSide(PortPos{Side::Right, int(-(m_right.count() + 1))}, p);
        }
    }

    struct PortIdBiMap {
        PortIdBiMap(Side _dir) : dir(_dir) {}
        Side dir;
        IdToPortMap idToPort;
        PortToIdMap portToId;

        unsigned count() const {
            assert(idToPort.size() == portToId.size());
            return idToPort.size();
        }

        PortIdBiMap(const PortIdBiMap&) = delete;  // no copying!
    };

    const SimPort* getPortAt(PortPos p) {
        auto& map = dirToMap(p.dir);
        if (map.idToPort.count(p.index))
            return map.idToPort.at(p.index);
        return nullptr;
    }

    /**
     * @brief movePort
     * @return true if port actually changed position
     */
    bool movePort(const SimPort* port, PortPos pos) {
        const auto* portAtPos = getPortAt(pos);
        if (portAtPos == port)
            return false;

        if (portAtPos != nullptr) {
            swapPorts(getPortAt(pos), port);
        } else {
            removePortFromSide(port);
            addPortToSide(pos, port);
        }
        return true;
    }

    void swapPorts(const SimPort* port1, const SimPort* port2) {
        auto pos1 = getPortPos(port1);
        auto pos2 = getPortPos(port2);

        removePortFromSide(port1);
        removePortFromSide(port2);
        addPortToSide(pos1, port2);
        addPortToSide(pos2, port1);
    }

    void addPortToSide(PortPos pos, const SimPort* port) {
        assert(m_portMap.count(port) > 0);
        assert((pos.validIndex() || pos.uninitialized()) &&
               "Ports cannot be on the edge of a component & todo: also check for other edge bound");
        auto& map = dirToMap(pos.dir);
        if (map.idToPort.count(pos.index)) {
            assert(false && "Port already at index");
        }
        map.idToPort[pos.index] = port;
        map.portToId[port] = pos.index;
        m_portMap[port] = &map;
    }

    void removePortFromSide(const SimPort* port) {
        auto* map = m_portMap.at(port);
        const int id = map->portToId.at(port);
        map->portToId.erase(port);
        map->idToPort.erase(id);
        m_portMap[port] = nullptr;
    }

    PortPos getPortPos(const SimPort* p) {
        auto* map = m_portMap.at(p);
        return {map->dir, map->portToId[p]};
    }

    inline PortIdBiMap& dirToMap(Side d) {
        switch (d) {
            case Side::Left:
                return m_left;
            case Side::Right:
                return m_right;
            case Side::Top:
                return m_top;
            case Side::Bottom:
                return m_bottom;
        }
    }

private:
    std::map<const SimPort*, PortIdBiMap*> m_portMap;
    PortIdBiMap m_left = PortIdBiMap(Side::Left);
    PortIdBiMap m_right = PortIdBiMap(Side::Right);
    PortIdBiMap m_top = PortIdBiMap(Side::Top);
    PortIdBiMap m_bottom = PortIdBiMap(Side::Bottom);
};

class GridComponent : public GraphicsBase {
    Q_OBJECT
public:
    GridComponent(SimComponent& c, GridComponent* parent);

    /**
     * @brief adjust
     * Attempt to expand the currently visible grid rect. Bounded by current minimum rect size.
     * @return true if any change to the currently visible grid rect was performed
     */
    bool adjust(const QPoint&);
    bool adjust(const QRect&);

    /**
     * @brief move
     * Attempts to move this gridcomponent to the desired pos in the given coordinate basis.
     * @return true if move was successfull (ie. if move was within bounds)
     */
    bool move(CPoint<CSys::Parent> pos);

    /**
     * @brief setExpanded
     * Change the current expansion state to @p state
     */
    void setExpanded(bool state);

    bool isExpanded() const { return m_expanded; }

    /**
     * @brief adjustPort
     * Attempt to move the position of @p port to @p pos
     * @return  whether the requested pos has been set for the port
     */
    bool adjustPort(SimPort* port, PortPos pos);

    PortPos getPortPos(const SimPort* port) const;

    const ComponentBorder& getBorder() const { return *m_border; }
    const QRect& getCurrentComponentRect() const;
    const QRect& getCurrentMinRect() const;

signals:
    void gridRectChanged();
    void portPosChanged(const SimPort* p);

protected:
    SimComponent& m_component;
    /**
     * @brief spreadPorts
     * Adjust all ports of the component such that they are evenly spread on a given face of the gridcomponent. Ports'
     * direction will not be changed
     */
    void spreadPorts();

private:
    /**
     * @brief childExpanded
     * Called by child components, signalling that they expanded which (may) require a rezing of the current minimum
     * component rect
     */
    void childExpanded();

private:
    /**
     * @brief spreadPortsOnSide
     * Spread all ports currently located on @p side
     */
    void spreadPortsOnSide(const Side& side);

    /**
     * @brief Rect update functions
     * @returns true if the given rect was modified
     */
    bool updateCurrentComponentRect(int dx, int dy);
    bool updateMinimumGridRect();
    bool updateSubcomponentBoundingRect();

    QRect& getCurrentComponentRectRef();

    /**
     * @brief Coordinate transformation functions
     */
    CPoint<CSys::Global> parentToGlobalCoords(CPoint<CSys::Parent> p);
    CPoint<CSys::Global> localToGlobalCoords(CPoint<CSys::Local> p);
    CPoint<CSys::Parent> localToParentCoords(CPoint<CSys::Local> p);

    /**
     * @brief moveInsideParent
     * Attempts to move this gridcomponent to the desired @p pos inside the parent.
     * @return true if move was successfull (ie. if move was within the parent bounds)
     */
    bool moveInsideParent(QPoint pos);

    bool parentContainsRect(const QRect& r) const;
    std::vector<GridComponent*> getGridSubcomponents() const;

    std::unique_ptr<ComponentBorder> m_border;

    /** current expansion state */
    bool m_expanded = false;

    /// Managed rectangles
    QRect m_currentExpandedRect;
    QRect m_currentContractedRect;
    QRect m_currentSubcomponentBoundingRect;
    QRect m_minimumGridRect;

    CPoint<CSys::Parent> m_relPos;  // Position in parent coordinates
};

}  // namespace vsrtl
