#include "vsrtl_placement.h"

namespace vsrtl {
namespace eda {

PhysicalPlacement::PhysicalPlacement(const std::vector<std::shared_ptr<ComponentTile>>& components)
    : m_components(components) {}

QRect PhysicalPlacement::componentBoundingRect() const {
    std::function<QRect(const std::shared_ptr<ComponentTile>&)> f = [](const auto& rr) { return rr.get()->rect(); };
    return boundingRectOfRectsF<QRect>(m_components, f);
}

std::shared_ptr<TileGraph> PhysicalPlacement::toTileGraph() const {
    std::shared_ptr<TileGraph> tg = std::make_shared<TileGraph>();

    QRect chipRect = componentBoundingRect();
    // Expand chiprect slightly, to reach (0,0) topleft and give equal spacing to the other faces.
    const int hSpacing = chipRect.x();
    const int vSpacing = chipRect.y();
    chipRect.adjust(-hSpacing, -vSpacing, hSpacing, vSpacing);

    // Check that a valid placement was received (all components contained within the chip boundary)
    std::vector<QRect> rects;
    std::transform(m_components.begin(), m_components.end(), std::back_inserter(rects),
                   [](const auto& c) { return c->rect(); });
    QRect br = boundingRectOfRects(rects);
    Q_ASSERT(chipRect == br || chipRect.contains(br));
    Q_ASSERT(chipRect.topLeft() == QPoint(0, 0));

    QList<Line> hz_bounding_lines, vt_bounding_lines, hz_tile_lines, vt_tile_lines;

    // Get horizontal and vertical bounding rectangle lines for all components on chip
    for (const auto& r : m_components) {
        hz_bounding_lines << getEdge(r->rect(), Direction::North) << getEdge(r->rect(), Direction::South);
        assert_valid_line(hz_bounding_lines.back());
        vt_bounding_lines << getEdge(r->rect(), Direction::West) << getEdge(r->rect(), Direction::East);
        assert_valid_line(vt_bounding_lines.back());
    }

    // ============== Component edge extrusion =================
    // This step extrudes the horizontal and vertical bounding rectangle lines for each component on the chip. The
    // extrusion will extend the line in each direction, until an obstacle is met (chip edges or other components)

    // Extrude horizontal lines
    for (const auto& h_line : hz_bounding_lines) {
        // Stretch line to chip boundary
        Line stretchedLine = Line({chipRect.left(), h_line.p1().y()}, {chipRect.right() + 1, h_line.p1().y()});
        assert_valid_line(stretchedLine);

        // Record the stretched line for debugging
        tg->stretchedLines.push_back(stretchedLine);

        // Narrow line until no boundaries are met
        for (const auto& v_line : vt_bounding_lines) {
            QPoint intersectPoint;
            if (stretchedLine.intersects(v_line, intersectPoint, IntersectType::Cross)) {
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
        assert_valid_line(stretchedLine);

        // Record the stretched line for debugging
        tg->stretchedLines.push_back(stretchedLine);

        // Narrow line until no boundaries are met
        for (const auto& h_line : hz_bounding_lines) {
            QPoint intersectPoint;
            // Intersecting lines must cross each other. This avoids intersections with a rectangles own sides
            if (h_line.intersects(stretchedLine, intersectPoint, IntersectType::Cross)) {
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
    hz_tile_lines << getEdge(chipRect, Direction::North) << getEdge(chipRect, Direction::South);
    vt_tile_lines << getEdge(chipRect, Direction::West) << getEdge(chipRect, Direction::East);

    tg->tileLines.insert(tg->tileLines.end(), hz_tile_lines.begin(), hz_tile_lines.end());
    tg->tileLines.insert(tg->tileLines.end(), vt_tile_lines.begin(), vt_tile_lines.end());

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
            assert_valid_line(vt_tile_line);
            assert_valid_line(hz_tile_line);

            if (hz_tile_line.intersects(vt_tile_line, tileBottom, IntersectType::OnEdge)) {
                // Intersection found (bottom left or right point of tile)

                // 1. Locate the point above the current intersection point (top right of tile)
                for (int hi_rev = hi - 1; hi_rev >= 0; hi_rev--) {
                    topHzLine = &hz_tile_lines[hi_rev];
                    if (topHzLine->intersects(vt_tile_line, tileTop, IntersectType::OnEdge)) {
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
                        if (hz_tile_line.intersects(vt_tile_lines[vi_rev], tileBottomRight, IntersectType::OnEdge)) {
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
                        if (hz_tile_line.intersects(vt_tile_lines[vi_rev], tileBottomLeft, IntersectType::OnEdge)) {
                            break;
                        }
                    }
                }

                // Set top left coordinate
                tileTopLeft = QPoint(tileBottomLeft.x(), tileTop.y());
                assert_valid_point(tileTopLeft);

                // Check whether the tile is enclosing a component.
                QRect newTileRect = QRect(tileTopLeft, tileBottomRight);
                assert_valid_rect(newTileRect);

                // Check whether the new tile encloses a component
                const auto componentInTileIt = std::find_if(m_components.begin(), m_components.end(),
                                                            [&newTileRect](const auto& routingComponent) {
                                                                QRect rrect = routingComponent->rect();
                                                                rrect.setBottomRight(realBottomRight(rrect));
                                                                return newTileRect == rrect;
                                                            });

                if (componentInTileIt == m_components.end()) {
                    // New tile was not a component, check if new tile has already been added
                    auto routingTiles = tg->vertices<RoutingTile>();
                    if (std::find_if(routingTiles.begin(), routingTiles.end(), [&newTileRect](const auto& tile) {
                            return tile->rect() == newTileRect;
                        }) == routingTiles.end()) {
                        // This is a new routing tile
                        auto* newTile = tg->addVertex(std::make_shared<RoutingTile>(newTileRect));

                        // Add tile to tileGroups
                        tileGroups[newTileRect.topLeft()].setTile(Corner::BottomRight, newTile);
                        tileGroups[newTileRect.bottomLeft()].setTile(Corner::TopRight, newTile);
                        tileGroups[newTileRect.topRight()].setTile(Corner::BottomLeft, newTile);
                        tileGroups[newTileRect.bottomRight()].setTile(Corner::TopLeft, newTile);
                    }
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
    for (auto& rc : m_components) {
        // The algorithm have failed if tileGroups does not contain an entry for each corner of all routing components
        const auto rect = rc->rect();
        Q_ASSERT(tileGroups.count(rect.topLeft()));
        Q_ASSERT(tileGroups.count(realTopRight(rect)));
        Q_ASSERT(tileGroups.count(realBottomRight(rect)));
        Q_ASSERT(tileGroups.count(realBottomLeft(rect)));
        rc->setTileAtEdge(Direction::North, tileGroups[rect.topLeft()].topright);
        rc->setTileAtEdge(Direction::West, tileGroups[rect.topLeft()].bottomleft);
        rc->setTileAtEdge(Direction::East, tileGroups[realTopRight(rect)].bottomright);
        rc->setTileAtEdge(Direction::South, tileGroups[realBottomLeft(rect)].bottomright);
    }

    tg->initialize();
    return tg;
}

std::set<RoutingTile*> gatherTilesInDirections(RoutingTile* origin, const std::set<Direction>& directions) {
    std::set<RoutingTile*> tiles;
    auto f = [&](Tile*, Tile* tileIt, Direction) {
        if (auto* rt = dynamic_cast<RoutingTile*>(tileIt)) {
            tiles.insert(rt);
            return true;
        }
        return false;
    };

    for (const auto d : directions) {
        origin->iterateInDirection(f, d);
    }
    tiles.insert(origin);
    return tiles;
}

unsigned MatrixPlacement::numRows(unsigned column) const {
    auto it = matrix.find(column);
    assert(matrix.count(column) && "Column not found");
    return it->second.size();
}

unsigned MatrixPlacement::height(unsigned* column) const {
    unsigned maxRows = 0;
    unsigned maxColumn = 0;
    for (auto& it : matrix) {
        if (it.second.size() > maxRows) {
            maxRows = it.second.size();
            maxColumn = it.first;
        }
    }
    if (column)
        *column = maxColumn;

    return maxRows;
}

std::shared_ptr<TileGraph> MatrixPlacement::toTileGraph() const {
    // We convert the matrix into a 2d grid interspersing RoutingTile's with ComponentTile's.
    auto tg = std::make_shared<TileGraph>();

    // Initialize the tilegraph with a border of tiles on the top- and left-hand side
    // x x x x
    // x - - -
    // x - - -
    // x - - -

    Tile* topLeftTile = tg->addVertex(std::make_shared<RoutingTile>(QRect(0, 0, 0, 0)));
    Tile* preTilePtr = topLeftTile;
    for (unsigned x = 0; x < width() * 2; x++) {
        auto* tile = tg->addVertex(std::make_shared<RoutingTile>());
        tile->setTileAtEdge(Direction::West, preTilePtr);
        preTilePtr = tile;
    }
    preTilePtr = topLeftTile;
    for (unsigned x = 0; x < height() * 2; x++) {
        auto* tile = tg->addVertex(std::make_shared<RoutingTile>());
        tile->setTileAtEdge(Direction::North, preTilePtr);
        preTilePtr = tile;
    }

    // Iterate through the matrix, and create either ComponentTiles (in case of components) or RoutingTiles.
    Tile* leftNeighbour = topLeftTile;
    for (unsigned y = 0; y < height(); y++) {
        // Get the leftmost tile in the next row down.
        leftNeighbour = leftNeighbour->getTileAtEdge(Direction::South);
        for (unsigned x = 0; x < width(); x++) {
            auto colIt = matrix.find(y);
            assert(colIt != matrix.end());
            Tile* newTile = nullptr;
            if (colIt->second.size() < y) {
                // No component tile exists here; create a routing tile instead
                newTile = tg->addVertex(std::make_shared<RoutingTile>());
            } else {
                GridComponent* component = colIt->second.at(y);
                newTile = tg->addVertex(std::make_shared<ComponentTile>(component));
            }
            // Connect new tile to its western and northern neighbours
            newTile->setTileAtEdge(Direction::West, leftNeighbour);
            auto* northNeighbour = newTile->traverseRoute({Direction::West, Direction::North, Direction::East});
            assert(northNeighbour);
            newTile->setTileAtEdge(Direction::North, northNeighbour);

            // The new tile is now the left neighbour.
            leftNeighbour = newTile;
        }
    }
    tg->initialize();
    return tg;
}

}  // namespace eda
}  // namespace vsrtl
