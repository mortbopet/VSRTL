#ifndef ROUTING_H
#define ROUTING_H

#include <QRect>
#include <map>
#include <set>
#include <vector>

#include "vsrtl_geometry.h"
#include "vsrtl_graphics_util.h"

// Various data-structures used during routing

namespace vsrtl {
class GridComponent;
class SimPort;

namespace eda {

class RoutingTile;

class RoutingComponent {
public:
    RoutingComponent(GridComponent* c) : gridComponent(c) {}
    GridComponent* gridComponent;
    QPoint pos;
    QRect rect() const;
    /**
     * Routing tileson each side of this component
     */
    RoutingTile* topTile;
    RoutingTile* leftTile;
    RoutingTile* rightTile;
    RoutingTile* bottomTile;
};

struct NetNode {
    std::shared_ptr<RoutingComponent> routingComponent;
    RoutingTile* tile = nullptr;
    SimPort* port = nullptr;
};

struct Route {
    Route(NetNode s, NetNode e) : start(s), end(e) {}
    NetNode start;
    NetNode end;
    std::vector<RoutingTile*> path;
};

class RoutingTile {
public:
    struct RoutePath {
        RoutingTile* tile = nullptr;
        Direction dir;
        int idx;
        QPoint from() const;
        QPoint to() const;
    };
    RoutingTile(const QRect& rect) : r(rect), h_cap(rect.width() - 1), v_cap(rect.height() - 1) { m_id = rr_ids++; }

    const QRect& rect() const { return r; }
    std::vector<RoutingTile*> adjacentTiles();
    int capacity(Direction dir) const;
    int remainingCap(Direction dir) const;
    int id() const { return m_id; }
    /**
     * @brief adjacentTile
     * @param rr
     * @param valid
     * @return the edge which @p rr abutts to this tile. If not abutting, @p valid is set to false.
     */
    Edge adjacentTile(const RoutingTile* rr, bool& valid) const;
    /**
     * @brief adjacentRowCol
     * like adjacentTile, but recursively checks all tiles in the row/column. If the target tile is found, returns the
     * edge that was traversed to find it.
     * @param is set to false if the target tile was not found in row/col.
     */
    Edge adjacentRowCol(const RoutingTile* rr, bool& valid) const;
    RoutingTile* getAdjacentTile(Edge edge);
    const RoutingTile* getAdjacentTile(Edge edge) const;

    void setTileAtEdge(Edge, RoutingTile*);
    void assignRoutes();
    RoutePath getPath(Route* route) const;
    void registerRoute(Route*, Direction);

    static inline bool cmpRoutingRegPtr(RoutingTile* a, RoutingTile* b) {
        if ((a == nullptr && b != nullptr) || (b == nullptr && a != nullptr))
            return false;
        if (a == nullptr && b == nullptr)
            return true;
        return a->r == b->r;
    }

    bool operator==(const RoutingTile& lhs) const;

private:
    bool adjacentRowColRec(const RoutingTile* rr, Edge dir) const;
    std::vector<Route*> verticalRoutes, horizontalRoutes;
    std::map<Route*, RoutePath> assignedRoutes;
    // A unique ID representing this routing tile
    int m_id;
    static int rr_ids;

    /**
     * Routing tiles on each row/column of this component
     */
    RoutingTile* top;
    RoutingTile* left;
    RoutingTile* right;
    RoutingTile* bottom;

    QRect r;    // tile size and position
    int h_cap;  // Horizontal capacity of tile
    int v_cap;  // Vertical capacity of tile
    int h_used = 0;
    int v_used = 0;
};

/**
 * @brief The tileGroup class
 * A tile group is the notion of the 4 tiles surrounding a horizontal and vertical intersection between tile group
 * boundaries.
 * When groups has been associated with all 4 corners, connecttile() may be called to make the tile groups at these
 * 4 corners aware of their adjacent tile groups in respect to this point.
 */
class TileGroup {
public:
    void connectTiles() {
        if (topleft != nullptr) {
            topleft->setTileAtEdge(Edge::Bottom, bottomleft);
            topleft->setTileAtEdge(Edge::Right, topright);
        }

        if (topright != nullptr) {
            topright->setTileAtEdge(Edge::Left, topleft);
            topright->setTileAtEdge(Edge::Bottom, bottomright);
        }
        if (bottomleft != nullptr) {
            bottomleft->setTileAtEdge(Edge::Top, topleft);
            bottomleft->setTileAtEdge(Edge::Right, bottomright);
        }
        if (bottomright != nullptr) {
            bottomright->setTileAtEdge(Edge::Left, bottomleft);
            bottomright->setTileAtEdge(Edge::Top, topright);
        }
    }
    void setTile(Corner, RoutingTile*);

    RoutingTile* topleft = nullptr;
    RoutingTile* topright = nullptr;
    RoutingTile* bottomleft = nullptr;
    RoutingTile* bottomright = nullptr;
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
    std::vector<std::unique_ptr<RoutingTile>> tiles;

    void dumpDotFile(const QString& path = QString()) const;

    // For debugging
    std::vector<Line> stretchedLines;
    std::vector<Line> tileLines;
};

WRAP_UNIQUEPTR(RoutingGraph)
RoutingGraphPtr createConnectivityGraph(Placement& placement);

/**
 * @brief The tileMap class
 * Data structure for retrieving the tile group which envelops the provided index.
 */
class TileMap {
public:
    TileMap(const RoutingGraph& tiles) {
        // tiles will be mapped to their lower-right corner in terms of indexing operations.
        // This is given the user of std::map::lower_bound to determine the map index
        for (const auto& tile : tiles.tiles) {
            const auto& bottomRight = tile->rect().bottomRight();
            tileMap[bottomRight.x()][bottomRight.y()] = tile.get();
        }
    }

    RoutingTile* lookup(const QPoint& index, Edge tieBreakVt = Edge::Left, Edge tieBreakHz = Edge::Top) const {
        return lookup(index.x(), index.y(), tieBreakVt, tieBreakHz);
    }

    RoutingTile* lookup(int x, int y, Edge tieBreakVt = Edge::Left, Edge tieBreakHz = Edge::Top) const {
        Q_ASSERT(tieBreakHz == Edge::Top || tieBreakHz == Edge::Bottom);
        Q_ASSERT(tieBreakVt == Edge::Left || tieBreakVt == Edge::Right);

        const auto& vertMap = tileMap.lower_bound(x + (tieBreakVt == Edge::Left ? 0 : 1));
        if (vertMap != tileMap.end()) {
            const auto& tileIt = vertMap->second.lower_bound(y + (tieBreakHz == Edge::Top ? 0 : 1));
            if (tileIt != vertMap->second.end()) {
                return tileIt->second;
            }
        }

        // Could not find a routing tile corresponding to the index
        return nullptr;
    }
    // Indexable tile map : {x coord. : {y coord. : Routingtile}}
    std::map<int, std::map<int, RoutingTile*>> tileMap;
};

using Net = std::vector<std::unique_ptr<Route>>;
WRAP_UNIQUEPTR(Net)
using Netlist = std::vector<NetPtr>;
WRAP_UNIQUEPTR(Netlist)

NetlistPtr createNetlist(Placement& placement, const TileMap& tileMap);

struct PRResult {
    Placement placement;
    RoutingGraphPtr tiles;
    NetlistPtr netlist;
};

}  // namespace eda
}  // namespace vsrtl

bool operator<(const QPoint& lhs, const QPoint& rhs);

#endif  // ROUTING_H
