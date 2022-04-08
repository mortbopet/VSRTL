#include "vsrtl_tilegraph.h"

#include "vsrtl_dotfile.h"
#include "vsrtl_gridcomponent.h"

#include <QDebug>

namespace vsrtl {
namespace eda {

int Tile::s_rr_ids = 0;

Tile::Tile(const QRect& rect) : r(rect) {
    m_id = s_rr_ids++;

    // One neighbour in each direction
    for (int i = 0; i < NDirections; i++) {
        addNeighbour(nullptr);
    }
}

ComponentTile::ComponentTile(GridComponent* c) : Tile(c->getCurrentComponentRect()), gridComponent(c) {}

void ComponentTile::updatePos(ComponentPos p) {
    switch (p) {
        case ComponentPos::Center: {
            auto componentSize = gridComponent->getCurrentComponentRect().size();
            QPoint offset = QPoint(componentSize.width(), componentSize.height()) / 2;
            gridComponent->setPos(rect().center() - offset);
            break;
        }
        case ComponentPos::TopLeft: {
            gridComponent->setPos(rect().topLeft());
            break;
        }
    }
}

void TileGraph::dumpDotFile(const QString& path) const {
    const auto realPath = path.isEmpty() ? "routinggraph.dot" : path;

    DotFile f(realPath.toStdString(), "RoutingGraph");
    for (const auto& tile : vertices<RoutingTile>()) {
        const std::string rid = std::to_string(tile->id());
        f.addVar(rid, rid);
    }

    for (const auto& tile : vertices<RoutingTile>()) {
        const std::string rid = std::to_string(tile->id());
        for (const auto& adjTile : tile->adjacentTiles()) {
            if (adjTile == nullptr) {
                continue;
            }
            if (auto* rtile = dynamic_cast<RoutingTile*>(adjTile)) {
                const std::string adjrid = std::to_string(rtile->id());
                f.addEdge(rid, adjrid);
            }
        }
    }

    f.dump();
}

void expandTileRecursively(Tile* tile, unsigned w, unsigned h, bool& didExpansion) {
    auto oldRect = tile->rect();
    if (w > oldRect.width()) {
        tile->setWidth(w);
        didExpansion = true;
        qDebug() << "Expanded tile " << tile->id() << " " << oldRect << " to " << tile->rect();
    }

    if (h > oldRect.height()) {
        tile->setHeight(h);
        didExpansion = true;
        qDebug() << "Expanded tile " << tile->id() << " " << oldRect << " to " << tile->rect();
    }

    // expand neighbours to the east/west
    for (auto* t : {tile->getTileAtEdge(Direction::West), tile->getTileAtEdge(Direction::East)}) {
        auto* rt = dynamic_cast<Tile*>(t);
        if (!rt || rt->rect().height() >= tile->rect().height())
            continue;
        expandTileRecursively(rt, rt->rect().width(), tile->rect().height(), didExpansion);
    }

    // expand neighbours to the north/south
    for (auto* t : {tile->getTileAtEdge(Direction::North), tile->getTileAtEdge(Direction::South)}) {
        auto* rt = dynamic_cast<Tile*>(t);
        if (!rt || rt->rect().width() >= tile->rect().width())
            continue;
        expandTileRecursively(rt, tile->rect().width(), rt->rect().height(), didExpansion);
    }

    assert_valid_rect(tile->rect());
}

TileMap::TileMap(const TileGraph& tiles) {
    // tiles will be mapped to their lower-right corner in terms of indexing operations.
    // This is given the user of std::map::lower_bound to determine the map index
    for (const auto& tile : tiles.vertices<RoutingTile>()) {
        const auto& bottomRight = tile->rect().bottomRight();
        tileMap[bottomRight.x()][bottomRight.y()] = tile;
    }
}

RoutingTile* TileMap::lookup(const QPoint& index, Direction tieBreakVt, Direction tieBreakHz) const {
    return lookup(index.x(), index.y(), tieBreakVt, tieBreakHz);
}

RoutingTile* TileMap::lookup(int x, int y, Direction tieBreakVt, Direction tieBreakHz) const {
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

/// Places all routing tiles relative to a starting tile (at 0,0) and the width and height of each tile.
static void placeTiles(RoutingTile* start, TileGraph& tg) {
    assert(start->pos() == QPoint(0, 0) && "Expected start tile to be at (0, 0)");
    std::set<Tile*> visited;
    auto vertices = tg.vertices<Tile>();
    auto unvisited = std::set<Tile*>(vertices.begin(), vertices.end());
    std::vector<Tile*> queue;

    queue.push_back(start);

    while (!queue.empty()) {
        auto* tile = queue.front();
        queue.erase(queue.begin());
        if (visited.count(tile))
            continue;
        visited.insert(tile);
        unvisited.erase(tile);

        if (auto* rt = dynamic_cast<Tile*>(tile->getTileAtEdge(Direction::East))) {
            rt->setPos(tile->rect().topRight());
            queue.push_back(rt);
            assert_valid_rect(rt->rect());
        }
        if (auto* rt = dynamic_cast<Tile*>(tile->getTileAtEdge(Direction::South))) {
            rt->setPos(tile->rect().bottomLeft());
            queue.push_back(rt);
            assert_valid_rect(rt->rect());
        }
    }

    // Some tiles may be left unvisited.
    // @TODO: For these, find a visited neighbour and try to place it relative to this. This can be done in an iterative
    // fashion, but should monitor for no change in the queue.
    for (auto t : unvisited) {
        if (!visited.count(t))
            qWarning() << "Tile " << t->rect() << "Was never visited, and";
    }
}

void placeTilesRec(RoutingTile* tile, std::set<RoutingTile*>& alreadyPlaced) {
    if (alreadyPlaced.count(tile)) {
        return;
    }

    alreadyPlaced.insert(tile);
    std::set<RoutingTile*> toIterate;
    if (auto* rt = dynamic_cast<RoutingTile*>(tile->neighbour(Direction::East))) {
        rt->setPos(tile->rect().topRight());
        toIterate.insert(rt);
        assert_valid_rect(rt->rect());
    }
    if (auto* rt = dynamic_cast<RoutingTile*>(tile->neighbour(Direction::West))) {
        rt->setPos(tile->rect().topLeft() - QPoint(rt->rect().width() - 1, 0));
        toIterate.insert(rt);
        assert_valid_rect(rt->rect());
    }
    if (auto* rt = dynamic_cast<RoutingTile*>(tile->neighbour(Direction::South))) {
        rt->setPos(tile->rect().bottomLeft());
        toIterate.insert(rt);
        assert_valid_rect(rt->rect());
    }
    if (auto* rt = dynamic_cast<RoutingTile*>(tile->neighbour(Direction::North))) {
        rt->setPos(tile->rect().topLeft() - QPoint(0, rt->rect().height() - 1));
        toIterate.insert(rt);
        assert_valid_rect(rt->rect());
    }

    for (const auto& rt : toIterate) {
        placeTilesRec(rt, alreadyPlaced);
    }
}

template <typename T>
std::set<T> setMinus(const std::set<T>& s1, const std::set<T>& s2) {
    std::set<T> diff;
    std::set_difference(s1.begin(), s1.end(), s2.begin(), s2.end(), std::inserter(diff, diff.begin()));
    return diff;
}

template <typename T>
void assertIsSubset(const std::set<T>& s1, const std::set<T>& s2) {
    auto diff = setMinus(s1, s2);
    Q_ASSERT(diff.size() == 0);
}

void TileGraph::initialize() {
    m_components = vertices<ComponentTile>();
    m_tileMap = std::make_unique<TileMap>(*this);
}

QRect TileGraph::boundingRect() const {
    assert(m_initialized && "tilegraph was not initialized post-construction!");
    std::function<QRect(Tile*)> f = [](const auto& rr) { return rr->rect(); };
    return boundingRectOfRectsF<QRect, Tile*>(vertices<Tile>(), f);
}
bool TileGraph::expandTiles() {
    assert(m_initialized && "tilegraph was not initialized post-construction!");
    QRect oldBR = boundingRect();
    // For each tile in the tilegraph, expand itself to fit the requested number of wires, and recursively expand its
    // neighbours if they do not correspond to the new size.

    bool didExpansion = false;
    for (auto& rt : vertices<RoutingTile>())
        expandTileRecursively(rt, rt->used(Orientation::Vertical), rt->used(Orientation::Horizontal), didExpansion);

    if (!didExpansion) {
        // No changes to the state of the tiles!
        return false;
    }

    // Tiles are repositioned based on their adjacency to one another. This is done recursively starting from the
    // top-left tile.
    RoutingTile* startTile = m_tileMap->lookup(QPoint(0, 0));
    Q_ASSERT(startTile);
    Q_ASSERT(startTile->neighbour(Direction::West) == nullptr);
    Q_ASSERT(startTile->neighbour(Direction::North) == nullptr);

    std::set<RoutingTile*> alreadyPlaced;
    startTile->setPos(QPoint(0, 0));
    // placeTilesRec(tile, alreadyPlaced);
    placeTiles(startTile, *this);

    // Sanity check that the bounding rect actually changed, if we said that we expanded some tile.
    Q_ASSERT(boundingRect() != oldBR);

    return true;
}

void TileGroup::setTile(Corner c, RoutingTile* tile) {
    switch (c) {
        case Corner::BottomLeft: {
            bottomleft = tile;
            return;
        }
        case Corner::BottomRight: {
            bottomright = tile;
            return;
        }
        case Corner::TopLeft: {
            topleft = tile;
            return;
        }
        case Corner::TopRight: {
            topright = tile;
            return;
        }
    }
}

RoutingTile::RoutePath RoutingTile::getPath(Route* route) const {
    auto it = m_assignedRoutes.find(route);
    Q_ASSERT(it != m_assignedRoutes.end());
    return it->second;
}

void Tile::setTileAtEdge(Direction e, Tile* tile) {
    Q_ASSERT(tile != this);
    neighbourRef(e) = tile;
    if (tile) {
        // Reflect edge setting (register this tile with the target tile).
        tile->neighbourRef(inv(e)) = this;
    }
}

Tile* Tile::getTileAtEdge(Direction e) {
    return static_cast<Tile*>(neighbour(e));
}

Tile* Tile::traverseRoute(const std::vector<Direction>& route) {
    Tile* current = this;
    for (auto dir : route) {
        current = current->getTileAtEdge(dir);
        if (!current)
            return nullptr;
    }
    return current;
}

QPoint RoutingTile::RoutePath::from() const {
    const auto r = tile->rect();
    /** @todo: direction needs to be changed to north/south/east/west, else this doesn't make sense*/
    QPoint p;
    switch (dir) {
        case Orientation::Horizontal:
            p = r.topLeft() + QPoint{0, idx};
            break;
        case Orientation::Vertical:
            p = r.topLeft() + QPoint{idx, 0};
            break;
            /*
        case Direction::North:
            p = r.bottomLeft() + QPoint{idx, 0};
            break;
        case Direction::West:
            p = r.topRight() + QPoint{0, idx};
            break;
*/
    }
    return p;
}

QPoint RoutingTile::RoutePath::to() const {
    const auto r = tile->rect();
    /** @todo: direction needs to be changed to north/south/east/west, else this doesn't make sense*/
    QPoint p;
    switch (dir) {
        case Orientation::Horizontal:
            p = r.topRight() + QPoint{0, idx};
            break;
        case Orientation::Vertical:
            p = r.bottomLeft() + QPoint{idx, 0};
            break;
            /*
        case Direction::North:
            p = r.bottomLeft() + QPoint{idx, 0};
            break;
        case Direction::West:
            p = r.topRight() + QPoint{0, idx};
            break;
*/
    }
    return p;
}

std::vector<Tile*> Tile::adjacentTiles() {
    return neighbours<Tile>([](Tile* t) { return t != nullptr; });
}

Direction Tile::isAdjacentDir(const Tile* rr, bool& valid) const {
    valid = true;
    const QRect r1 = this->rect();
    const QRect r2 = rr->rect();
    if (r1.y() < r2.y()) {
        return Direction::South;
    } else if (r1.y() > r2.y()) {
        return Direction::North;
    } else if (r1.x() < r2.x()) {
        return Direction::East;
    } else if (r1.x() > r2.x()) {
        return Direction::West;
    }
    valid = false;
    return {};
}

Direction Tile::isAdjacentTile(const Tile* rr, bool& valid) const {
    valid = true;
    for (const auto& dir : allDirections) {
        if (rr == neighbour<Tile>(dir)) {
            return dir;
        }
    }
    valid = false;
    return Direction();
}

Direction Tile::adjacentRowCol(const Tile* otherTile, bool& valid) {
    valid = false;
    Direction retEdge;
    RowColItFunc iterFunc = [&](Tile* /*origTile*/, Tile* itTile, Direction e) {
        if (itTile == otherTile) {
            // @param otherTile was found to be in the current row/column being iterated
            retEdge = e;
            valid = true;
            return false;
        } else {
            // Continue iteration
            return true;
        }
    };

    iterateInDirections(iterFunc, allDirections);
    return retEdge;
}

void Tile::iterateInDirection(const RowColItFunc& f, Direction edge) {
    iterateInDirections(f, {edge});
}

void Tile::iterateInDirections(const RowColItFunc& f, const std::set<Direction>& edges) {
    for (auto dir : edges) {
        if (auto* adjTile = neighbour<Tile>(dir)) {
            if (!adjTile->iterateDirectionRec(this, f, dir)) {
                return;
            }
        }
    }
}

bool Tile::iterateDirectionRec(Tile* origTile, const RowColItFunc& f, Direction dir) {
    if (f(origTile, this, dir)) {
        auto* adjTile = neighbour<Tile>(dir);
        if (adjTile) {
            return adjTile->iterateDirectionRec(origTile, f, dir);
        } else {
            // No further tiles in the current direction, but we continue iteration in next row/col
            return true;
        }
    }

    // Do not continue iteration in next row/col
    return false;
}

const std::set<Route*>& RoutingTile::routes(Orientation dir) const {
    switch (dir) {
        case Orientation::Horizontal:
            return m_horizontalRoutes;
        case Orientation::Vertical:
            return m_verticalRoutes;
    }

    Q_UNREACHABLE();
}

void RoutingTile::registerRoute(Route* r, Orientation d) {
    if (d == Orientation::Horizontal) {
        m_horizontalRoutes.insert(r);
    } else {
        m_verticalRoutes.insert(r);
    }
}

int RoutingTile::capacity(Orientation dir) const {
    if (dir == Orientation::Horizontal) {
        return rect().height() - 1;
    } else {
        return rect().width() - 1;
    }
}

int RoutingTile::remainingCap(Orientation dir) const {
    if (dir == Orientation::Horizontal) {
        return capacity(Orientation::Horizontal) - m_horizontalRoutes.size();
    } else {
        return capacity(Orientation::Vertical) - m_verticalRoutes.size();
    }
}

int RoutingTile::used(Orientation dir) const {
    if (dir == Orientation::Horizontal) {
        return m_horizontalRoutes.size();
    } else {
        return m_verticalRoutes.size();
    }
}

namespace {
inline bool cmpTilePtr(const Tile* a, const Tile* b) {
    if ((a == nullptr && b != nullptr) || (b == nullptr && a != nullptr))
        return false;
    if (a == nullptr && b == nullptr)
        return true;
    return a->rect() == b->rect();
}
}  // namespace

bool Tile::operator==(const Tile& lhs) const {
    if (!cmpTilePtr(neighbour<Tile>(Direction::North), lhs.neighbour<Tile>(Direction::North)))
        return false;
    if (!cmpTilePtr(neighbour<Tile>(Direction::South), lhs.neighbour<Tile>(Direction::South)))
        return false;
    if (!cmpTilePtr(neighbour<Tile>(Direction::West), lhs.neighbour<Tile>(Direction::West)))
        return false;
    if (!cmpTilePtr(neighbour<Tile>(Direction::East), lhs.neighbour<Tile>(Direction::East)))
        return false;

    return rect() == lhs.rect();
}

void RoutingTile::assignRoutes() {
    const float hz_diff = static_cast<float>(capacity(Orientation::Horizontal)) / (m_horizontalRoutes.size() + 1);
    const float vt_diff = static_cast<float>(capacity(Orientation::Vertical)) / (m_verticalRoutes.size() + 1);
    float hz_pos = hz_diff;
    float vt_pos = vt_diff;
    for (const auto& route : m_horizontalRoutes) {
        m_assignedRoutes[route] = {this, Orientation::Horizontal, static_cast<int>(hz_pos)};
        hz_pos += hz_diff;
    }
    for (const auto& route : m_verticalRoutes) {
        m_assignedRoutes[route] = {this, Orientation::Vertical, static_cast<int>(vt_pos)};
        vt_pos += vt_diff;
    }
}

}  // namespace eda
}  // namespace vsrtl

bool operator<(const QPoint& lhs, const QPoint& rhs) {
    return (lhs.x() < rhs.x()) || ((lhs.x() == rhs.x()) && (lhs.y() < rhs.y()));
}
