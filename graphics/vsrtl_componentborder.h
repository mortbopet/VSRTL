#pragma once

#include <QPoint>

#include "cereal/cereal.hpp"
#include "cereal/types/map.hpp"

#include "../interface/vsrtl_interface.h"

namespace vsrtl {

enum class Side { Left, Right, Top, Bottom };
struct PortPos {
  Side side;
  int index;
  bool validIndex() const { return index > 0; }

  bool operator==(const PortPos &rhs) {
    return (this->index == rhs.index) && (this->side == rhs.side);
  }
};

class ComponentBorder {
public:
  using IdToPortMap = std::map<int, const SimPort *>;
  using PortToIdMap = std::map<const SimPort *, int>;
  using NameToPortMap = std::map<std::string, const SimPort *>;

  ComponentBorder(const SimComponent *c) {
    std::set<SimPort *> placedPorts;

    for (const auto &p : c->getPorts<SimPort::PortType::in>()) {
      initPlacePortOnSide(Side::Left, p, placedPorts);
    }
    for (const auto &p : c->getPorts<SimPort::PortType::out>()) {
      initPlacePortOnSide(Side::Right, p, placedPorts);
    }
  }

  void initPlacePortOnSide(Side s, SimPort *p, std::set<SimPort *> &placed) {
    if (placed.count(p))
      return;

    m_portMap[p] = nullptr;
    const auto &sideMap = sideToMap(s);
    addPortToSide(PortPos{s, static_cast<int>(-(sideMap.count() + 1))}, p);
    m_namePortMap[p->getName()] = p;
    placed.insert(p);
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

  const SimPort *getPortAt(PortPos p) {
    auto &map = sideToMap(p.side);
    if (map.idToPort.count(p.index))
      return map.idToPort.at(p.index);
    return nullptr;
  }

  /**
   * @brief movePort
   * @return list of ports moved in the operation
   */
  std::vector<const SimPort *> movePort(const SimPort *port, PortPos pos) {
    std::vector<const SimPort *> portsMoved;
    const auto *portAtPos = getPortAt(pos);
    if (portAtPos == port)
      return {};

    if (portAtPos != nullptr) {
      portsMoved.push_back(portAtPos);
      swapPorts(getPortAt(pos), port);
    } else {
      removePortFromSide(port);
      addPortToSide(pos, port);
    }
    portsMoved.push_back(port);
    return portsMoved;
  }

  void swapPorts(const SimPort *port1, const SimPort *port2) {
    auto pos1 = getPortPos(port1);
    auto pos2 = getPortPos(port2);

    removePortFromSide(port1);
    removePortFromSide(port2);
    addPortToSide(pos1, port2);
    addPortToSide(pos2, port1);
  }

  void addPortToSide(PortPos pos, const SimPort *port) {
    assert(m_portMap.count(port) > 0);
    auto &map = sideToMap(pos.side);
    if (map.idToPort.count(pos.index)) {
      assert(false && "Port already at index");
    }
    map.idToPort[pos.index] = port;
    map.portToId[port] = pos.index;
    m_portMap[port] = &map;
  }

  void removePortFromSide(const SimPort *port) {
    auto *map = m_portMap.at(port);
    const int id = map->portToId.at(port);
    map->portToId.erase(port);
    map->idToPort.erase(id);
    m_portMap[port] = nullptr;
  }

  PortPos getPortPos(const SimPort *p) {
    auto *map = m_portMap.at(p);
    return {map->dir, map->portToId[p]};
  }

  inline PortIdBiMap &sideToMap(Side d) {
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
    Q_UNREACHABLE();
  }

  template <class Archive>
  void serialize(Archive &archive) {
    std::map<Side, std::map<std::string, unsigned>> portPosSerialMap;
    // Create a mapping between port names and their positions
    for (const auto &pm : {m_left, m_right, m_top, m_bottom}) {
      for (const auto &p : pm.idToPort) {
        portPosSerialMap[pm.dir][p.second->getName()] = p.first;
      }
    }

    try {
      archive(cereal::make_nvp("Component border", portPosSerialMap));
    } catch (const cereal::Exception &e) {
      /// @todo: build an error report
    }

    // Locate ports via. their names and set their positions as per the
    // serialized archive.
    for (const auto &pm : portPosSerialMap) {
      for (const auto &p : pm.second) {
        if (!m_namePortMap.count(p.first))
          continue;

        PortPos pos;
        pos.side = pm.first;
        pos.index = p.second;

        const SimPort *port = m_namePortMap.at(p.first);
        movePort(port, pos);
      }
    }
  }

private:
  NameToPortMap m_namePortMap;
  std::map<const SimPort *, PortIdBiMap *> m_portMap;
  PortIdBiMap m_left = PortIdBiMap(Side::Left);
  PortIdBiMap m_right = PortIdBiMap(Side::Right);
  PortIdBiMap m_top = PortIdBiMap(Side::Top);
  PortIdBiMap m_bottom = PortIdBiMap(Side::Bottom);
};

} // namespace vsrtl
