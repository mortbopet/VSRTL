#ifndef ROUTING_H
#define ROUTING_H

#include <QRect>
#include <map>
#include <set>
#include <vector>

#include "vsrtl_geometry.h"
#include "vsrtl_graph.h"
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
    t1->isAdjacentTile(t2, valid);
    Q_ASSERT(valid);
}

class Tile : public Vertex {
public:
    Tile(const QRect& rect);

    /**
     * @brief adjacentTile
     * @param rr
     * @param valid
     * @return the edge which @p rr abutts to this tile. If not abutting, @p valid is set to false.
     */
    Direction isAdjacentTile(const Tile* rr, bool& valid) const;
    /**
     * @brief adjacentDir
     * Like the above but returns the direction which @p rr is to this tile (geometry based rather than connectivity
     * based).
     */
    Direction isAdjacentDir(const Tile* rr, bool& valid) const;

    /**
     * @brief adjacentRowCol
     * like adjacentTile, but recursively checks all tiles in the row/column. If the target tile is found, returns the
     * edge that was traversed to find it.
     * @param is set to false if the target tile was not found in row/col.
     */
    Direction adjacentRowCol(const Tile* rr, bool& valid);
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
        const Direction e = isAdjacentDir(t, valid);
        Q_ASSERT(valid);
        iterateInDirections(f, {e});
        return intTiles;
    }

    void setTileAtEdge(Direction, Tile*);
    Tile* getTileAtEdge(Direction);
    int id() const { return m_id; }

    bool operator==(const Tile& lhs) const;
    virtual QRect rect() const { return r; }
    void setWidth(unsigned w) { r.setWidth(w); }
    void setHeight(unsigned h) { r.setHeight(h); }
    void setPos(const QPoint& pos) { r.moveTo(pos); }
    QPoint pos() const { return r.topLeft(); }

    // The row/column iteration function shall return true if iteration should continue
    /** @brief RowColItFunc
     * @param Tile*: Origin tile where this iteration started
     * @param Tile*: Tile currently being iterated
     * @param Edge: Edge which this iteration originated from in the original tile
     * @return true if iteration should continue
     */
    using RowColItFunc = std::function<bool(Tile*, Tile*, Direction)>;
    void iterateInDirection(const RowColItFunc& f, Direction edge);
    void iterateInDirections(const RowColItFunc& f, const std::set<Direction>& edges);

protected:
    bool iterateDirectionRec(Tile* origTile, const RowColItFunc& f, Direction dir);

private:
    bool adjacentRowColRec(const Tile* rr, Direction dir) const;

    // A unique ID representing this routing tile
    int m_id;
    // Routing id counter.
    static int s_rr_ids;

    QRect r;  // tile size and position
};

class ComponentTile : public Tile {
public:
    ComponentTile(GridComponent* c);
    GridComponent* gridComponent;

    enum class ComponentPos { TopLeft, Center };
    void updatePos(ComponentPos p);
};

struct NetNode {
    std::shared_ptr<ComponentTile> componentTile;
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
    RoutingTile(const QRect& rect) : Tile(rect) {}

    int capacity(Orientation dir) const;
    int remainingCap(Orientation dir) const;
    int used(Orientation dir) const;
    void assignRoutes();
    RoutePath getPath(Route* route) const;
    void registerRoute(Route*, Orientation);

    const std::set<Route*>& routes(Orientation dir) const;

private:
    std::set<Route*> m_verticalRoutes, m_horizontalRoutes;
    std::map<Route*, RoutePath> m_assignedRoutes;
};

/**
 * @brief The tileGroup class
 * A tile group is the notion of the 4 tiles surrounding a horizontal and vertical intersection between tile
 * group boundaries. When groups has been associated with all 4 corners, connecttile() may be called to make the
 * tile groups at these 4 corners aware of their adjacent tile groups in respect to this point.
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

class Placement {
public:
    Placement(const std::vector<std::shared_ptr<ComponentTile>>& components = {}, const QRect& chipRect = QRect());

    QRect componentBoundingRect() const;

    /**
     * @brief tileBasedPlacement
     * Places all components based on the geometry of their surrounding routing tiles.
     */
    void doTileBasedPlacement();
    void fitChipRectToComponents();
    void addComponent(const std::shared_ptr<ComponentTile>& c) { m_components.push_back(c); }
    const std::vector<std::shared_ptr<ComponentTile>>& components() const { return m_components; }
    const QRect& chipRect() const { return m_chipRect; }
    void setChipRect(const QRect& rect) { m_chipRect = rect; }

private:
    std::vector<std::shared_ptr<ComponentTile>> m_components;
    QRect m_chipRect;
};

class TileGraph;
/**
 * @brief The tileMap class
 * Data structure for retrieving the tile group which envelops the provided index.
 */
class TileMap {
public:
    TileMap(const TileGraph& tiles);

    RoutingTile* lookup(const QPoint& index, Direction tieBreakVt = Direction::West,
                        Direction tieBreakHz = Direction::North) const;

    RoutingTile* lookup(int x, int y, Direction tieBreakVt = Direction::West,
                        Direction tieBreakHz = Direction::North) const;

    // Indexable tile map : {x coord. : {y coord. : Routingtile}}
    std::map<int, std::map<int, RoutingTile*>> tileMap;
};

class TileGraph : public Graph<Tile> {
public:
    TileGraph(const std::shared_ptr<Placement>& placement);

    void dumpDotFile(const QString& path = QString()) const;

    /**
     * @brief expandTiles
     * Post route registration, expands all tiles to fit the maximum number of vertical/horizontal routes
     * registerred in each row/column of the routable area.
     * Returns true if any tile was expanded.
     */
    bool expandTiles();

    QRect boundingRect() const;

    // For debugging
    std::vector<Line> stretchedLines;
    std::vector<Line> tileLines;

private:
    std::unique_ptr<TileMap> m_tileMap;
};

}  // namespace eda
}  // namespace vsrtl

bool operator<(const QPoint& lhs, const QPoint& rhs);

#endif  // ROUTING_H
