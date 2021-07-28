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

template <typename T1, typename T2>
void assertAdjacentTiles(const T1* t1, const T2* t2) {
    bool valid;
    t1->adjacentTile(t2, valid);
    Q_ASSERT(valid);
}

class Tile {
public:
    /**
     * @brief adjacentTile
     * @param rr
     * @param valid
     * @return the edge which @p rr abutts to this tile. If not abutting, @p valid is set to false.
     */
    Direction adjacentTile(const Tile* rr, bool& valid) const;
    /**
     * @brief adjacentDir
     * Like the above but returns the direction which @p rr is to this tile (geometry based rather than connectivity
     * based).
     */
    Direction adjacentDir(const Tile* rr, bool& valid) const;

    /**
     * @brief adjacentRowCol
     * like adjacentTile, but recursively checks all tiles in the row/column. If the target tile is found, returns the
     * edge that was traversed to find it.
     * @param is set to false if the target tile was not found in row/col.
     */
    Direction adjacentRowCol(const Tile* rr, bool& valid);
    Tile* getAdjacentTile(Direction edge);
    const Tile* getAdjacentTile(Direction edge) const;
    std::vector<Tile*> adjacentTiles();

    /**
     * @brief intermediateTiles
     * @return an ordered vector of tiles between this tile and @p t. If this and @p t are not in the same row/column or
     * obstructed by some component, returns an empty set.
     */
    template <typename T>
    std::vector<T*> intermediateTiles(const T* t) {
        std::vector<T*> intTiles;
        RowColItFunc f = [&](Tile* /*origTile*/, Tile* itTile, Direction /*e*/) {
            if (itTile == t) {
                // finish iteration
                return false;
            } else {
                if (auto* itTTile = dynamic_cast<T*>(itTile)) {
                    intTiles.push_back(itTTile);
                } else {
                    return false;
                }
            }
            return true;
        };
        bool valid;
        const Direction e = adjacentDir(t, valid);
        Q_ASSERT(valid);
        iterateFromEdges(f, {e});
        return intTiles;
    }

    void setTileAtEdge(Direction, Tile*);

    bool operator==(const Tile& lhs) const;
    virtual QRect rect() const = 0;

protected:
    // The row/column iteration function shall return true if iteration should continue
    /** @brief RowColItFunc
     * @param Tile*: Origin tile where this iteration started
     * @param Tile*: Tile currently being iterated
     * @param Edge: Edge which this iteration originated from in the original tile
     * @return true if iteration should continue
     */
    using RowColItFunc = std::function<bool(Tile*, Tile*, Direction)>;
    void iterateFromEdge(const RowColItFunc& f, Direction edge);
    void iterateFromEdges(const RowColItFunc& f, const std::set<Direction>& edges);
    bool iterateEdgeRec(Tile* origTile, const RowColItFunc& f, Direction dir);

    /**
     * Routing tiles on each row/column of this component
     */
    Tile* top;
    Tile* left;
    Tile* right;
    Tile* bottom;

private:
    bool adjacentRowColRec(const Tile* rr, Direction dir) const;
};

class RoutingComponent : public Tile {
public:
    RoutingComponent(GridComponent* c) : gridComponent(c) {}
    GridComponent* gridComponent;
    QPoint pos;
    QRect rect() const override;
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

class RoutingTile : public Tile {
public:
    struct RoutePath {
        RoutingTile* tile = nullptr;
        Orientation dir;
        int idx;
        QPoint from() const;
        QPoint to() const;
    };
    RoutingTile(const QRect& rect) : r(rect), h_cap(rect.width() - 1), v_cap(rect.height() - 1) { m_id = rr_ids++; }

    QRect rect() const override { return r; }
    int capacity(Orientation dir) const;
    int remainingCap(Orientation dir) const;
    int id() const { return m_id; }
    void assignRoutes();
    RoutePath getPath(Route* route) const;
    void registerRoute(Route*, Orientation);

    const std::set<Route*>& routes(Orientation dir) const;

private:
    std::set<Route*> m_verticalRoutes, m_horizontalRoutes;
    std::map<Route*, RoutePath> m_assignedRoutes;
    // A unique ID representing this routing tile
    int m_id;
    static int rr_ids;

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
            topleft->setTileAtEdge(Direction::South, bottomleft);
            topleft->setTileAtEdge(Direction::East, topright);
        }

        if (topright != nullptr) {
            topright->setTileAtEdge(Direction::West, topleft);
            topright->setTileAtEdge(Direction::South, bottomright);
        }
        if (bottomleft != nullptr) {
            bottomleft->setTileAtEdge(Direction::North, topleft);
            bottomleft->setTileAtEdge(Direction::East, bottomright);
        }
        if (bottomright != nullptr) {
            bottomright->setTileAtEdge(Direction::West, bottomleft);
            bottomright->setTileAtEdge(Direction::North, topright);
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

    RoutingTile* lookup(const QPoint& index, Direction tieBreakVt = Direction::West,
                        Direction tieBreakHz = Direction::North) const {
        return lookup(index.x(), index.y(), tieBreakVt, tieBreakHz);
    }

    RoutingTile* lookup(int x, int y, Direction tieBreakVt = Direction::West,
                        Direction tieBreakHz = Direction::North) const {
        Q_ASSERT(tieBreakHz == Direction::North || tieBreakHz == Direction::South);
        Q_ASSERT(tieBreakVt == Direction::West || tieBreakVt == Direction::East);

        const auto& vertMap = tileMap.lower_bound(x + (tieBreakVt == Direction::West ? 0 : 1));
        if (vertMap != tileMap.end()) {
            const auto& tileIt = vertMap->second.lower_bound(y + (tieBreakHz == Direction::North ? 0 : 1));
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
Orientation directionBetweenRRs(RoutingTile* from, RoutingTile* to, Orientation def = Orientation::Horizontal);

struct PRResult {
    Placement placement;
    RoutingGraphPtr tiles;
    NetlistPtr netlist;
};

}  // namespace eda
}  // namespace vsrtl

bool operator<(const QPoint& lhs, const QPoint& rhs);

#endif  // ROUTING_H
