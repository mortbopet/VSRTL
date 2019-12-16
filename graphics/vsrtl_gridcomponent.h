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
    unsigned index;
};

class PortPosMap {
public:
    using IdToPortMap = std::map<unsigned, const SimPort*>;
    using PortToIdMap = std::map<const SimPort*, unsigned>;

    PortPosMap(const SimComponent& c) {
        for (const auto& p : c.getPorts<SimPort::Direction::in>()) {
            addPort(PortPos{Side::Left, static_cast<unsigned>(m_left.count())}, p);
        }
        for (const auto& p : c.getPorts<SimPort::Direction::out>()) {
            addPort(PortPos{Side::Right, static_cast<unsigned>(m_right.count())}, p);
        }
    }

    struct PortIdBiMap {
        PortIdBiMap(Side _dir) : dir(_dir) {}
        Side dir;
        IdToPortMap idToPort;
        PortToIdMap portToId;

        size_t count() const {
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
            removePort(port);
            addPort(pos, port);
        }
        return true;
    }

    void swapPorts(const SimPort* port1, const SimPort* port2) {
        auto pos1 = getPortPos(port1);
        auto pos2 = getPortPos(port2);

        removePort(port1);
        removePort(port2);
        addPort(pos1, port2);
        addPort(pos2, port1);
    }

    void addPort(PortPos pos, const SimPort* port) {
        auto& map = dirToMap(pos.dir);
        if (map.idToPort.count(pos.index)) {
            assert(false && "Port already at index");
        }
        map.idToPort[pos.index] = port;
        map.portToId[port] = pos.index;
    }

    void removePort(const SimPort* port) {
        auto& map = getPortMap(port);
        const unsigned id = map.portToId.at(port);
        map.portToId.erase(port);
        map.idToPort.erase(id);
    }

    PortIdBiMap& getPortMap(const SimPort* p) {
        std::vector<PortIdBiMap*> maps = {&m_left, &m_right, &m_top, &m_bottom};
        for (auto& m : maps) {
            if (m->portToId.count(p))
                return *m;
        }
        assert(false);
    }

    PortPos getPortPos(const SimPort* p) {
        auto& map = getPortMap(p);
        return {map.dir, map.portToId[p]};
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
    bool adjust(int dx, int dy);

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

    /**
     * @brief adjustPort
     * Attempt to move the position of @p port to @p pos
     * @return  whether the requested pos has been set for the port
     */
    bool adjustPort(SimPort* port, PortPos pos);

    const PortPosMap& getPortPosMap() const { return *m_portPosMap; }

signals:
    void expansionStateChanged(bool isExpanded);
    void portPosChanged(const SimPort* p);

protected:
    SimComponent& m_component;

private:
    /**
     * @brief spreadPorts
     * Adjust all ports of the component such that they are evenly spread on a given face of the gridcomponent. Ports'
     * direction will not be changed
     */
    void spreadPorts();

    /**
     * @brief spreadPortsOnSide
     * Spread all ports currently located on @p side
     */
    void spreadPortsOnSide(const Side& side);

    void updateCurrentComponentRect(int dx, int dy);
    void updateMinimumGridRect();
    void updateSubcomponentBoundingRect();

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

    QRect& getCurrentComponentRect();
    const QRect& getCurrentMinRect();

    bool parentContainsRect(const QRect& r) const;
    std::vector<GridComponent*> getGridSubcomponents() const;

    std::unique_ptr<PortPosMap> m_portPosMap;

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
