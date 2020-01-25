#pragma once

#include <QPoint>

#include "../interface/vsrtl_interface.h"

namespace vsrtl {

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

        // PortIdBiMap(const PortIdBiMap&) = delete;  // no copying!
    };

    const SimPort* getPortAt(PortPos p) {
        auto& map = dirToMap(p.dir);
        if (map.idToPort.count(p.index))
            return map.idToPort.at(p.index);
        return nullptr;
    }

    /**
     * @brief movePort
     * @return list of ports moved in the operation
     */
    std::vector<const SimPort*> movePort(const SimPort* port, PortPos pos) {
        std::vector<const SimPort*> portsMoved;
        const auto* portAtPos = getPortAt(pos);
        if (portAtPos == port)
            return {};

        if (portAtPos != nullptr) {
            portsMoved.push_back(getPortAt(pos));
            swapPorts(getPortAt(pos), port);
        } else {
            removePortFromSide(port);
            addPortToSide(pos, port);
        }
        portsMoved.push_back(port);
        return portsMoved;
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

}  // namespace vsrtl
