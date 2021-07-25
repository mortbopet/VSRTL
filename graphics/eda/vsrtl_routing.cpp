#include "vsrtl_routing.h"

#include "vsrtl_dotfile.h"
#include "vsrtl_gridcomponent.h"

namespace vsrtl {
namespace eda {

int RoutingTile::rr_ids = 0;

QRect RoutingComponent::rect() const {
    auto r = gridComponent->getCurrentComponentRect();
    r.moveTo(pos);
    return r;
}

NetlistPtr createNetlist(Placement& placement, const TileMap& tileMap) {
    auto netlist = std::make_unique<Netlist>();
    for (const auto& routingComponent : placement.components) {
        for (const auto& outputPort : routingComponent->gridComponent->getComponent()->getOutputPorts()) {
            // Note: terminal position currently is fixed to right => output, left => input
            auto net = std::make_unique<Net>();
            NetNode source;
            source.port = outputPort;
            source.routingComponent = routingComponent;

            // Get source port grid position and adjust based on the placement of that component
            const QPoint portPosLocal =
                routingComponent->gridComponent->getPortGridPos(outputPort) + routingComponent->pos;
            source.tile = tileMap.lookup(portPosLocal, Edge::Right);
            Q_ASSERT(source.tile != nullptr);
            for (const auto& sinkPort : outputPort->getOutputPorts()) {
                NetNode sink;
                sink.port = sinkPort;
                GridComponent* sinkGridComponent = sinkPort->getParent()->getGraphic<GridComponent>();
                Q_ASSERT(sinkGridComponent);
                // Lookup routing component for sink component graphic
                auto rc_i = std::find_if(
                    placement.components.begin(), placement.components.end(),
                    [&sinkGridComponent](const auto& rc) { return rc->gridComponent == sinkGridComponent; });

                if (rc_i == placement.components.end()) {
                    /** @todo: connected port is the parent component (i.e., an in- or output port of the parent
                     * component */
                    continue;
                }
                sink.routingComponent = *rc_i;

                // Get sink port grid position and adjust based on the placement of that component
                sink.tile = tileMap.lookup(
                    sink.routingComponent->gridComponent->getPortGridPos(sinkPort) + sink.routingComponent->pos,
                    Edge::Left);
                Q_ASSERT(sink.tile != nullptr);
                net->push_back(std::make_unique<Route>(source, sink));
            }
            netlist->push_back(std::move(net));
        }
    }
    return netlist;
}

void RoutingGraph::dumpDotFile(const QString& path) const {
    const auto realPath = path.isEmpty() ? "routinggraph.dot" : path;

    DotFile f(realPath.toStdString(), "RoutingGraph");
    for (const auto& tile : tiles) {
        const std::string rid = std::to_string(tile.get()->id());
        f.addVar(rid, rid);
    }

    for (const auto& tile : tiles) {
        const std::string rid = std::to_string(tile.get()->id());
        for (const auto& adjTile : tile->adjacentTiles()) {
            if (adjTile == nullptr) {
                continue;
            }
            const std::string adjrid = std::to_string(adjTile->id());
            f.addEdge(rid, adjrid);
        }
    }

    f.dump();
}

RoutingGraphPtr createConnectivityGraph(Placement& placement) {
    // Check that a valid placement was received (all components contained within the chip boundary)
    std::vector<QRect> rects;
    std::transform(placement.components.begin(), placement.components.end(), std::back_inserter(rects),
                   [](const auto& c) { return c->rect(); });
    Q_ASSERT(placement.chipRect.contains(boundingRectOfRects(rects)));
    Q_ASSERT(placement.chipRect.topLeft() == QPoint(0, 0));

    RoutingGraphPtr tiles = std::make_unique<RoutingGraph>();
    const auto& chipRect = placement.chipRect;

    QList<Line> hz_bounding_lines, vt_bounding_lines, hz_tile_lines, vt_tile_lines;

    // Get horizontal and vertical bounding rectangle lines for all components on chip
    for (const auto& r : placement.components) {
        hz_bounding_lines << getEdge(r->rect(), Edge::Top) << getEdge(r->rect(), Edge::Bottom);
        vt_bounding_lines << getEdge(r->rect(), Edge::Left) << getEdge(r->rect(), Edge::Right);
    }

    // ============== Component edge extrusion =================
    // This step extrudes the horizontal and vertical bounding rectangle lines for each component on the chip. The
    // extrusion will extend the line in each direction, until an obstacle is met (chip edges or other components)

    // Extrude horizontal lines
    for (const auto& h_line : hz_bounding_lines) {
        // Stretch line to chip boundary
        Line stretchedLine = Line({chipRect.left(), h_line.p1().y()}, {chipRect.right() + 1, h_line.p1().y()});

        // Record the stretched line for debugging
        tiles->stretchedLines.push_back(stretchedLine);

        // Narrow line until no boundaries are met
        for (const auto& v_line : vt_bounding_lines) {
            QPoint intersectPoint;
            if (stretchedLine.intersect(v_line, intersectPoint, IntersectType::Cross)) {
                // Contract based on point closest to original line segment
                if ((intersectPoint - h_line.p1()).manhattanLength() <
                    (intersectPoint - h_line.p2()).manhattanLength()) {
                    stretchedLine.setP1(intersectPoint);
                } else {
                    stretchedLine.setP2(intersectPoint);
                }
            }
        }

        // Add the stretched and now boundary analyzed line to the tile lines
        if (!hz_tile_lines.contains(stretchedLine))
            hz_tile_lines << stretchedLine;
    }

    // Extrude vertical lines
    for (const auto& line : vt_bounding_lines) {
        // Stretch line to chip boundary
        Line stretchedLine = Line({line.p1().x(), chipRect.top()}, {line.p1().x(), chipRect.bottom() + 1});

        // Record the stretched line for debugging
        tiles->stretchedLines.push_back(stretchedLine);

        // Narrow line until no boundaries are met
        for (const auto& h_line : hz_bounding_lines) {
            QPoint intersectPoint;
            // Intersecting lines must cross each other. This avoids intersections with a rectangles own sides
            if (h_line.intersect(stretchedLine, intersectPoint, IntersectType::Cross)) {
                // Contract based on point closest to original line segment
                if ((intersectPoint - line.p1()).manhattanLength() < (intersectPoint - line.p2()).manhattanLength()) {
                    stretchedLine.setP1(intersectPoint);
                } else {
                    stretchedLine.setP2(intersectPoint);
                }
            }
        }

        // Add the stretched and now boundary analyzed line to the vertical tile lines
        if (!vt_tile_lines.contains(stretchedLine))
            vt_tile_lines << stretchedLine;
    }

    // Add chip boundaries to tile lines. The chiprect cannot use RoutingComponent::rect so we'll manually adjust the
    // edge lines.
    hz_tile_lines << getEdge(chipRect, Edge::Top) << getEdge(chipRect, Edge::Bottom);
    vt_tile_lines << getEdge(chipRect, Edge::Left) << getEdge(chipRect, Edge::Right);

    tiles->tileLines.insert(tiles->tileLines.end(), hz_tile_lines.begin(), hz_tile_lines.end());
    tiles->tileLines.insert(tiles->tileLines.end(), vt_tile_lines.begin(), vt_tile_lines.end());

    // Sort bounding lines
    // Top to bottom
    std::sort(hz_tile_lines.begin(), hz_tile_lines.end(),
              [](const auto& a, const auto& b) { return a.p1().y() < b.p1().y(); });
    // Left to right
    std::sort(vt_tile_lines.begin(), vt_tile_lines.end(),
              [](const auto& a, const auto& b) { return a.p1().x() < b.p1().x(); });

    // ============ Routing tile creation =================== //

    // Maintain a map of tiles around each intersecion point in the graph. This will aid in connecting the graph after
    // the tiles are found
    std::map<QPoint, TileGroup> tileGroups;

    // Bounding lines are no longer needed
    hz_bounding_lines.clear();
    vt_bounding_lines.clear();

    // Find intersections between horizontal and vertical tile lines, and create corresponding routing tiles.
    QPoint tileBottomLeft, tileBottomRight, tileTopLeft;
    QPoint tileBottom, tileTop;
    Line* topHzLine = nullptr;
    for (int hi = 1; hi < hz_tile_lines.size(); hi++) {
        for (int vi = 1; vi < vt_tile_lines.size(); vi++) {
            const auto& vt_tile_line = vt_tile_lines[vi];
            const auto& hz_tile_line = hz_tile_lines[hi];
            if (hz_tile_line.intersect(vt_tile_line, tileBottom, IntersectType::OnEdge)) {
                // Intersection found (bottom left or right point of tile)

                // 1. Locate the point above the current intersection point (top right of tile)
                for (int hi_rev = hi - 1; hi_rev >= 0; hi_rev--) {
                    topHzLine = &hz_tile_lines[hi_rev];
                    if (topHzLine->intersect(vt_tile_line, tileTop, IntersectType::OnEdge)) {
                        // Intersection found (top left or right point of tile)
                        break;
                    }
                }
                // Determine whether bottom right or bottom left point was found
                if (vt_tile_line.p1().x() == hz_tile_line.p1().x()) {
                    // Bottom left corner was found
                    tileBottomLeft = tileBottom;
                    // Locate bottom right of tile
                    for (int vi_rev = vi + 1; vi_rev < vt_tile_lines.size(); vi_rev++) {
                        if (hz_tile_line.intersect(vt_tile_lines[vi_rev], tileBottomRight, IntersectType::OnEdge)) {
                            break;
                        }
                    }
                } else {
                    // Bottom right corner was found.
                    // Check whether topHzLine terminates in the topRightCorner - in this case, there can be no routing
                    // tile here (the tile would pass into a component). No check is done for when the bottom left
                    // corner is found,given that the algorithm iterates from vertical lines, left to right.
                    if (topHzLine->p1().x() == tileBottom.x()) {
                        continue;
                    }
                    tileBottomRight = tileBottom;
                    // Locate bottom left of tile
                    for (int vi_rev = vi - 1; vi_rev >= 0; vi_rev--) {
                        if (hz_tile_line.intersect(vt_tile_lines[vi_rev], tileBottomLeft, IntersectType::OnEdge)) {
                            break;
                        }
                    }
                }

                // Set top left coordinate
                tileTopLeft = QPoint(tileBottomLeft.x(), tileTop.y());

                // Check whether the tile is enclosing a component.
                QRect newTileRect = QRect(tileTopLeft, tileBottomRight);

                // Check whether the new tile encloses a component
                const auto componentInTileIt = std::find_if(placement.components.begin(), placement.components.end(),
                                                            [&newTileRect](const auto& routingComponent) {
                                                                QRect rrect = routingComponent->rect();
                                                                rrect.setBottomRight(realBottomRight(rrect));
                                                                return newTileRect == rrect;
                                                            });

                if (componentInTileIt == placement.components.end()) {
                    // New tile was not a component, check if new tile has already been added
                    if (std::find_if(tiles->tiles.begin(), tiles->tiles.end(), [&newTileRect](const auto& tile) {
                            return tile.get()->rect() == newTileRect;
                        }) == tiles->tiles.end())

                        // This is a new routing tile
                        tiles->tiles.push_back(std::make_unique<RoutingTile>(newTileRect));
                    RoutingTile* newtile = tiles->tiles.rbegin()->get();

                    // Add tile to tileGroups
                    tileGroups[newTileRect.topLeft()].setTile(Corner::BottomRight, newtile);
                    tileGroups[newTileRect.bottomLeft()].setTile(Corner::TopRight, newtile);
                    tileGroups[newTileRect.topRight()].setTile(Corner::BottomLeft, newtile);
                    tileGroups[newTileRect.bottomRight()].setTile(Corner::TopLeft, newtile);
                }
            }
        }
    }

    // =================== Connectivity graph connection ========================== //
    for (auto& iter : tileGroups) {
        TileGroup& group = iter.second;
        group.connectTiles();
    }

    // ======================= Routing Tile Association ======================= //
    // Step 1: Associate with directly adjacent neighbours
    for (auto& rc : placement.components) {
        // The algorithm have failed if tileGroups does not contain an entry for each corner of all routing components
        const auto rect = rc->rect();
        Q_ASSERT(tileGroups.count(rect.topLeft()));
        Q_ASSERT(tileGroups.count(realTopRight(rect)));
        Q_ASSERT(tileGroups.count(realBottomRight(rect)));
        Q_ASSERT(tileGroups.count(realBottomLeft(rect)));
        rc->topTile = tileGroups[rect.topLeft()].topright;
        rc->leftTile = tileGroups[rect.topLeft()].bottomleft;
        rc->rightTile = tileGroups[realTopRight(rect)].bottomright;
        rc->bottomTile = tileGroups[realBottomLeft(rect)].bottomright;
    }

    /*
    // Step 2: Associate with all regions in row/column
    for (auto& rc : placement.components) {
        // The algorithm have failed if tileGroups does not contain an entry for each corner of all routing components
        RoutingComponent* rcptr = rc.get();
        auto regionTraverseGen = [&](const std::function<const std::set<RoutingRegion*>&(RoutingRegion*)>&
                                         regionGetter) {
            std::function<void(std::set<RoutingRegion*>&, const std::set<RoutingRegion*>&)> recursiveTraverseRegions =
                [&](std::set<RoutingRegion*>& base, const std::set<RoutingRegion*>& regions) {
                    base.insert(regions.begin(), regions.end());
                    for (const auto& r : regions) {
                        recursiveTraverseRegions(base, regionGetter(r));
                    }
                };
            return recursiveTraverseRegions;
        };

        regionTraverseGen([&](RoutingRegion* rr) { return rr-> })

            do {
            auto& regions = rcptr->topRegions;
        }
        while (regionPtr = regionPtr->topRegions)

            const auto rect = rc->rect();
        Q_ASSERT(tileGroups.count(rect.topLeft()));
        Q_ASSERT(tileGroups.count(realTopRight(rect)));
        Q_ASSERT(tileGroups.count(realBottomRight(rect)));
        Q_ASSERT(tileGroups.count(realBottomLeft(rect)));
        rc->topRegion = tileGroups[rect.topLeft()].topright;
        rc->leftRegion = tileGroups[rect.topLeft()].bottomleft;
        rc->rightRegion = tileGroups[realTopRight(rect)].bottomright;
        rc->bottomRegion = tileGroups[realBottomLeft(rect)].bottomright;
    }
    */

    return tiles;
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
    auto it = assignedRoutes.find(route);
    Q_ASSERT(it != assignedRoutes.end());
    return it->second;
}

void RoutingTile::setTileAtEdge(Edge e, RoutingTile* tile) {
    switch (e) {
        case Edge::Top: {
            top = tile;
            return;
        }
        case Edge::Bottom: {
            bottom = tile;
            return;
        }
        case Edge::Left: {
            left = tile;
            return;
        }
        case Edge::Right: {
            right = tile;
            return;
        }
    }
}

QPoint RoutingTile::RoutePath::from() const {
    const auto r = tile->rect();
    /** @todo: direction needs to be changed to north/south/east/west, else this doesn't make sense*/
    QPoint p;
    switch (dir) {
        case Direction::Horizontal:
            p = r.topLeft() + QPoint{0, idx};
            break;
        case Direction::Vertical:
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
        case Direction::Horizontal:
            p = r.topRight() + QPoint{0, idx};
            break;
        case Direction::Vertical:
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

std::vector<RoutingTile*> RoutingTile::adjacentTiles() {
    std::vector<RoutingTile*> tiles;
    for (auto* t : {top, bottom, left, right}) {
        if (t != nullptr) {
            tiles.push_back(t);
        }
    }
    return tiles;
}

Edge RoutingTile::adjacentTile(const RoutingTile* rr, bool& valid) const {
    valid = true;
    if (rr == top) {
        return Edge::Top;
    } else if (rr == right) {
        return Edge::Right;
    } else if (rr == left) {
        return Edge::Left;
    } else if (rr == bottom) {
        return Edge::Bottom;
    }
    valid = false;
    return Edge();
}

Edge RoutingTile::adjacentRowCol(const RoutingTile* rr, bool& valid) const {
    Q_ASSERT(rr != this);
    valid = true;
    for (auto dir : {Edge::Bottom, Edge::Top, Edge::Left, Edge::Right}) {
        if (auto adjTile = getAdjacentTile(dir)) {
            if (adjTile->adjacentRowColRec(rr, dir)) {
                return dir;
            }
        }
    }

    valid = false;
    return {};
}

bool RoutingTile::adjacentRowColRec(const RoutingTile* rr, Edge dir) const {
    if (this == rr) {
        return true;
    }
    auto adjTile = getAdjacentTile(dir);
    if (adjTile) {
        return adjTile->adjacentRowColRec(rr, dir);
    } else {
        return false;
    }
}

const RoutingTile* RoutingTile::getAdjacentTile(Edge edge) const {
    switch (edge) {
        case Edge::Top:
            return top;
        case Edge::Bottom:
            return bottom;
        case Edge::Left:
            return left;
        case Edge::Right:
            return right;
    }
    return nullptr;
}

RoutingTile* RoutingTile::getAdjacentTile(Edge edge) {
    return const_cast<RoutingTile*>(const_cast<const RoutingTile*>(this)->getAdjacentTile(edge));
}

void RoutingTile::registerRoute(Route* r, Direction d) {
    if (d == Direction::Horizontal) {
        horizontalRoutes.push_back(r);
    } else {
        verticalRoutes.push_back(r);
    }
}

int RoutingTile::capacity(Direction dir) const {
    if (dir == Direction::Horizontal) {
        return h_cap;
    } else {
        return v_cap;
    }
}

int RoutingTile::remainingCap(Direction dir) const {
    if (dir == Direction::Horizontal) {
        return h_cap - horizontalRoutes.size();
    } else {
        return v_cap - verticalRoutes.size();
    }
}

bool RoutingTile::operator==(const RoutingTile& lhs) const {
    if (!cmpRoutingRegPtr(top, lhs.top))
        return false;
    if (!cmpRoutingRegPtr(bottom, lhs.bottom))
        return false;
    if (!cmpRoutingRegPtr(left, lhs.left))
        return false;
    if (!cmpRoutingRegPtr(right, lhs.right))
        return false;

    return r == lhs.r;
}

void RoutingTile::assignRoutes() {
    const float hz_diff = static_cast<float>(h_cap) / (horizontalRoutes.size() + 1);
    const float vt_diff = static_cast<float>(v_cap) / (verticalRoutes.size() + 1);
    float hz_pos = hz_diff;
    float vt_pos = vt_diff;
    for (const auto& route : horizontalRoutes) {
        assignedRoutes[route] = {this, Direction::Horizontal, static_cast<int>(hz_pos)};
        hz_pos += hz_diff;
    }
    for (const auto& route : verticalRoutes) {
        assignedRoutes[route] = {this, Direction::Vertical, static_cast<int>(vt_pos)};
        vt_pos += vt_diff;
    }
}

Direction directionBetweenRRs(const RoutingTile* from, const RoutingTile* to, Direction def) {
    if (from == nullptr) {
        return def;
    } else {
        bool valid = true;
        Direction direction = edgeToDirection(from->adjacentRowCol(to, valid));
        Q_ASSERT(valid);
        return direction;
    }
}

}  // namespace eda
}  // namespace vsrtl

bool operator<(const QPoint& lhs, const QPoint& rhs) {
    return (lhs.x() < rhs.x()) || ((lhs.x() == rhs.x()) && (lhs.y() < rhs.y()));
}
