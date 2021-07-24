#ifndef ROUTING_H
#define ROUTING_H

#include <QRect>
#include <map>
#include <vector>

#include "vsrtl_geometry.h"
#include "vsrtl_graphics_util.h"

// Various data-structures used during routing

namespace vsrtl {
class GridComponent;
class SimPort;

namespace eda {

class RoutingRegion;

class RoutingComponent {
public:
    RoutingComponent(GridComponent* c) : gridComponent(c) {}
    GridComponent* gridComponent;
    QPoint pos;
    QRect rect() const;
    /**
     * @brief topRegion
     * Routing regions on each face of this component
     */
    RoutingRegion* topRegion = nullptr;
    RoutingRegion* leftRegion = nullptr;
    RoutingRegion* rightRegion = nullptr;
    RoutingRegion* bottomRegion = nullptr;
};

struct NetNode {
    std::shared_ptr<RoutingComponent> routingComponent;
    RoutingRegion* region = nullptr;
    SimPort* port = nullptr;
};

struct Route {
    Route(NetNode s, NetNode e) : start(s), end(e) {}
    NetNode start;
    NetNode end;
    std::vector<RoutingRegion*> path;
};

class RoutingRegion {
public:
    struct RoutePath {
        RoutingRegion* region = nullptr;
        Direction dir;
        int idx;
        QPoint from() const;
        QPoint to() const;
    };
    RoutingRegion(const QRect& rect) : r(rect), h_cap(rect.width() - 1), v_cap(rect.height() - 1) { m_id = rr_ids++; }

    const QRect& rect() const { return r; }
    const std::vector<RoutingRegion*> adjacentRegions();
    int capacity(Direction dir) const;
    int remainingCap(Direction dir) const;
    int id() const { return m_id; }
    /**
     * @brief adjacentRegion
     * @param rr
     * @param valid
     * @return the edge which @p rr abutts to this routing region. If not abutting, @p valid is set to false.
     */
    Edge adjacentRegion(const RoutingRegion* rr, bool& valid) const;

    void setRegion(Edge, RoutingRegion*);
    void assignRoutes();
    RoutePath getPath(Route* route) const;
    void registerRoute(Route*, Direction);

    static inline bool cmpRoutingRegPtr(RoutingRegion* a, RoutingRegion* b) {
        if ((a == nullptr && b != nullptr) || (b == nullptr && a != nullptr))
            return false;
        if (a == nullptr && b == nullptr)
            return true;
        return a->r == b->r;
    }

    bool operator==(const RoutingRegion& lhs) const;

private:
    std::vector<Route*> verticalRoutes, horizontalRoutes;
    std::map<Route*, RoutePath> assignedRoutes;
    // A unique ID representing this routing region
    int m_id;
    static int rr_ids;

    // Adjacent region groups
    RoutingRegion* top = nullptr;
    RoutingRegion* bottom = nullptr;
    RoutingRegion* left = nullptr;
    RoutingRegion* right = nullptr;

    QRect r;    // Region size and position
    int h_cap;  // Horizontal capacity of routing region
    int v_cap;  // Vertical capacity of routing region
    int h_used = 0;
    int v_used = 0;
};

/**
 * @brief The RegionGroup class
 * A region group is the notion of the 4 regions surrounding a horizontal and vertical intersection between region group
 * boundaries.
 * When groups has been associated with all 4 corners, connectRegion() may be called to make the region groups at these
 * 4 corners aware of their adjacent region groups in respect to this point.
 */
class RegionGroup {
public:
    void connectRegions() {
        if (topleft != nullptr) {
            topleft->setRegion(Edge::Bottom, bottomleft);
            topleft->setRegion(Edge::Right, topright);
        }

        if (topright != nullptr) {
            topright->setRegion(Edge::Left, topleft);
            topright->setRegion(Edge::Bottom, bottomright);
        }
        if (bottomleft != nullptr) {
            bottomleft->setRegion(Edge::Top, topleft);
            bottomleft->setRegion(Edge::Right, bottomright);
        }
        if (bottomright != nullptr) {
            bottomright->setRegion(Edge::Left, bottomleft);
            bottomright->setRegion(Edge::Top, topright);
        }
    }
    void setRegion(Corner, RoutingRegion*);

    RoutingRegion* topleft = nullptr;
    RoutingRegion* topright = nullptr;
    RoutingRegion* bottomleft = nullptr;
    RoutingRegion* bottomright = nullptr;
};

struct Placement {
    QRect chipRect;
    std::vector<std::shared_ptr<RoutingComponent>> components;
    QRect componentBoundingRect() const {
        std::function<QRect(const std::shared_ptr<RoutingComponent>&)> f = [](const auto& rr) {
            return rr.get()->rect();
        };
        return boundingRectOfRectsF<QRect>(components, f);
    }
};

#define WRAP_UNIQUEPTR(type) using type##Ptr = std::unique_ptr<type>;
class RoutingGraph {
public:
    std::vector<std::unique_ptr<RoutingRegion>> regions;

    void dumpDotFile(const QString& path = QString()) const;

    // For debugging
    std::vector<Line> stretchedLines;
    std::vector<Line> regionLines;
};

WRAP_UNIQUEPTR(RoutingGraph)
RoutingGraphPtr createConnectivityGraph(Placement& placement);

/**
 * @brief The RegionMap class
 * Data structure for retrieving the region group which envelops the provided index.
 */
class RegionMap {
public:
    RegionMap(const RoutingGraph& regions) {
        // Regions will be mapped to their lower-right corner in terms of indexing operations.
        // This is given the user of std::map::lower_bound to determine the map index
        for (const auto& region : regions.regions) {
            const auto& bottomRight = region->rect().bottomRight();
            regionMap[bottomRight.x()][bottomRight.y()] = region.get();
        }
    }

    RoutingRegion* lookup(const QPoint& index, Edge tieBreakVt = Edge::Left, Edge tieBreakHz = Edge::Top) const {
        return lookup(index.x(), index.y(), tieBreakVt, tieBreakHz);
    }

    RoutingRegion* lookup(int x, int y, Edge tieBreakVt = Edge::Left, Edge tieBreakHz = Edge::Top) const {
        Q_ASSERT(tieBreakHz == Edge::Top || tieBreakHz == Edge::Bottom);
        Q_ASSERT(tieBreakVt == Edge::Left || tieBreakVt == Edge::Right);

        const auto& vertMap = regionMap.lower_bound(x + (tieBreakVt == Edge::Left ? 0 : 1));
        if (vertMap != regionMap.end()) {
            const auto& regionIt = vertMap->second.lower_bound(y + (tieBreakHz == Edge::Top ? 0 : 1));
            if (regionIt != vertMap->second.end()) {
                return regionIt->second;
            }
        }

        // Could not find a routing region corresponding to the index
        return nullptr;
    }
    // Indexable region map : {x coord. : {y coord. : RoutingRegion}}
    std::map<int, std::map<int, RoutingRegion*>> regionMap;
};

using Net = std::vector<std::unique_ptr<Route>>;
WRAP_UNIQUEPTR(Net)
using Netlist = std::vector<NetPtr>;
WRAP_UNIQUEPTR(Netlist)

NetlistPtr createNetlist(Placement& placement, const RegionMap& regionMap);

struct PRResult {
    Placement placement;
    RoutingGraphPtr regions;
    NetlistPtr netlist;
};

}  // namespace eda
}  // namespace vsrtl

bool operator<(const QPoint& lhs, const QPoint& rhs);

#endif  // ROUTING_H
