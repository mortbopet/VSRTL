#include "vsrtl_routing.h"

#include "vsrtl_gridcomponent.h"

namespace vsrtl {
namespace eda {

int RoutingRegion::rr_ids = 0;

QRect RoutingComponent::rect() const {
    auto r = gridComponent->getCurrentComponentRect();
    r.moveTo(pos);
    return r;
}

NetlistPtr createNetlist(Placement& placement, const RegionMap& regionMap) {
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
            source.region = regionMap.lookup(portPosLocal, Edge::Right);
            Q_ASSERT(source.region != nullptr);
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
                sink.region = regionMap.lookup(
                    sink.routingComponent->gridComponent->getPortGridPos(sinkPort) + sink.routingComponent->pos,
                    Edge::Left);
                Q_ASSERT(sink.region != nullptr);
                net->push_back(std::make_unique<Route>(source, sink));
            }
            netlist->push_back(std::move(net));
        }
    }
    return netlist;
}

RoutingRegionsPtr createConnectivityGraph(Placement& placement) {
    // Check that a valid placement was received (all components contained within the chip boundary)
    std::vector<QRect> rects;
    std::transform(placement.components.begin(), placement.components.end(), std::back_inserter(rects),
                   [](const auto& c) { return c->rect(); });
    Q_ASSERT(placement.chipRect.contains(boundingRectOfRects(rects)));
    Q_ASSERT(placement.chipRect.topLeft() == QPoint(0, 0));

    RoutingRegionsPtr regions = std::make_unique<RoutingRegions>();
    const auto& chipRect = placement.chipRect;

    QList<Line> hz_bounding_lines, vt_bounding_lines, hz_region_lines, vt_region_lines;

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
        regions->stretchedLines.push_back(stretchedLine);

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

        // Add the stretched and now boundary analyzed line to the region lines
        if (!hz_region_lines.contains(stretchedLine))
            hz_region_lines << stretchedLine;
    }

    // Extrude vertical lines
    for (const auto& line : vt_bounding_lines) {
        // Stretch line to chip boundary
        Line stretchedLine = Line({line.p1().x(), chipRect.top()}, {line.p1().x(), chipRect.bottom() + 1});

        // Record the stretched line for debugging
        regions->stretchedLines.push_back(stretchedLine);

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

        // Add the stretched and now boundary analyzed line to the vertical region lines
        if (!vt_region_lines.contains(stretchedLine))
            vt_region_lines << stretchedLine;
    }

    // Add chip boundaries to region lines. The chiprect cannot use RoutingComponent::rect so we'll manually adjust the
    // edge lines.
    hz_region_lines << getEdge(chipRect, Edge::Top) << getEdge(chipRect, Edge::Bottom);
    vt_region_lines << getEdge(chipRect, Edge::Left) << getEdge(chipRect, Edge::Right);

    regions->regionLines.insert(regions->regionLines.end(), hz_region_lines.begin(), hz_region_lines.end());
    regions->regionLines.insert(regions->regionLines.end(), vt_region_lines.begin(), vt_region_lines.end());

    // Sort bounding lines
    // Top to bottom
    std::sort(hz_region_lines.begin(), hz_region_lines.end(),
              [](const auto& a, const auto& b) { return a.p1().y() < b.p1().y(); });
    // Left to right
    std::sort(vt_region_lines.begin(), vt_region_lines.end(),
              [](const auto& a, const auto& b) { return a.p1().x() < b.p1().x(); });

    // ============ Routing region creation =================== //

    // Maintain a map of regions around each intersecion point in the graph. This will aid in connecting the graph after
    // the regions are found
    std::map<QPoint, RegionGroup> regionGroups;

    // Bounding lines are no longer needed
    hz_bounding_lines.clear();
    vt_bounding_lines.clear();

    // Find intersections between horizontal and vertical region lines, and create corresponding routing regions.
    QPoint regionBottomLeft, regionBottomRight, regionTopLeft;
    QPoint regionBottom, regionTop;
    Line* topHzLine = nullptr;
    for (int hi = 1; hi < hz_region_lines.size(); hi++) {
        for (int vi = 1; vi < vt_region_lines.size(); vi++) {
            const auto& vt_region_line = vt_region_lines[vi];
            const auto& hz_region_line = hz_region_lines[hi];
            if (hz_region_line.intersect(vt_region_line, regionBottom, IntersectType::OnEdge)) {
                // Intersection found (bottom left or right point of region)

                // 1. Locate the point above the current intersection point (top right of region)
                for (int hi_rev = hi - 1; hi_rev >= 0; hi_rev--) {
                    topHzLine = &hz_region_lines[hi_rev];
                    if (topHzLine->intersect(vt_region_line, regionTop, IntersectType::OnEdge)) {
                        // Intersection found (top left or right point of region)
                        break;
                    }
                }
                // Determine whether bottom right or bottom left point was found
                if (vt_region_line.p1().x() == hz_region_line.p1().x()) {
                    // Bottom left corner was found
                    regionBottomLeft = regionBottom;
                    // Locate bottom right of region
                    for (int vi_rev = vi + 1; vi_rev < vt_region_lines.size(); vi_rev++) {
                        if (hz_region_line.intersect(vt_region_lines[vi_rev], regionBottomRight,
                                                     IntersectType::OnEdge)) {
                            break;
                        }
                    }
                } else {
                    // Bottom right corner was found.
                    // Check whether topHzLine terminates in the topRightCorner - in this case, there can be no routing
                    // region here (the region would pass into a component). No check is done for when the bottom left
                    // corner is found,given that the algorithm iterates from vertical lines, left to right.
                    if (topHzLine->p1().x() == regionBottom.x()) {
                        continue;
                    }
                    regionBottomRight = regionBottom;
                    // Locate bottom left of region
                    for (int vi_rev = vi - 1; vi_rev >= 0; vi_rev--) {
                        if (hz_region_line.intersect(vt_region_lines[vi_rev], regionBottomLeft,
                                                     IntersectType::OnEdge)) {
                            break;
                        }
                    }
                }

                // Set top left coordinate
                regionTopLeft = QPoint(regionBottomLeft.x(), regionTop.y());

                // Check whether the region is enclosing a component.
                QRect newRegionRect = QRect(regionTopLeft, regionBottomRight);

                // Check whether the new region encloses a component
                const auto componentInRegionIt = std::find_if(placement.components.begin(), placement.components.end(),
                                                              [&newRegionRect](const auto& routingComponent) {
                                                                  QRect rrect = routingComponent->rect();
                                                                  rrect.setBottomRight(realBottomRight(rrect));
                                                                  return newRegionRect == rrect;
                                                              });

                if (componentInRegionIt == placement.components.end()) {
                    // New region was not a component, check if new region has already been added
                    if (std::find_if(regions->regions.begin(), regions->regions.end(),
                                     [&newRegionRect](const auto& region) {
                                         return region.get()->rect() == newRegionRect;
                                     }) == regions->regions.end())

                        // This is a new routing region
                        regions->regions.push_back(std::make_unique<RoutingRegion>(newRegionRect));
                    RoutingRegion* newRegion = regions->regions.rbegin()->get();

                    // Add region to regionGroups
                    regionGroups[newRegionRect.topLeft()].setRegion(Corner::BottomRight, newRegion);
                    regionGroups[newRegionRect.bottomLeft()].setRegion(Corner::TopRight, newRegion);
                    regionGroups[newRegionRect.topRight()].setRegion(Corner::BottomLeft, newRegion);
                    regionGroups[newRegionRect.bottomRight()].setRegion(Corner::TopLeft, newRegion);
                }
            }
        }
    }

    // =================== Connectivity graph connection ========================== //
    for (auto& iter : regionGroups) {
        RegionGroup& group = iter.second;
        group.connectRegions();
    }

    // ======================= Routing Region Association ======================= //
    for (auto& rc : placement.components) {
        // The algorithm have failed if regionGroups does not contain an entry for each corner of all routing components
        const auto rect = rc->rect();
        Q_ASSERT(regionGroups.count(rect.topLeft()));
        Q_ASSERT(regionGroups.count(realTopRight(rect)));
        Q_ASSERT(regionGroups.count(realBottomRight(rect)));
        Q_ASSERT(regionGroups.count(realBottomLeft(rect)));
        rc->topRegion = regionGroups[rect.topLeft()].topright;
        rc->leftRegion = regionGroups[rect.topLeft()].bottomleft;
        rc->rightRegion = regionGroups[realTopRight(rect)].bottomright;
        rc->bottomRegion = regionGroups[realBottomLeft(rect)].bottomright;
    }

    return regions;
}

void RegionGroup::setRegion(Corner c, RoutingRegion* region) {
    switch (c) {
        case Corner::BottomLeft: {
            bottomleft = region;
            return;
        }
        case Corner::BottomRight: {
            bottomright = region;
            return;
        }
        case Corner::TopLeft: {
            topleft = region;
            return;
        }
        case Corner::TopRight: {
            topright = region;
            return;
        }
    }
}

RoutingRegion::RoutePath RoutingRegion::getPath(Route* route) const {
    auto it = assignedRoutes.find(route);
    Q_ASSERT(it != assignedRoutes.end());
    return it->second;
}

void RoutingRegion::setRegion(Edge e, RoutingRegion* region) {
    switch (e) {
        case Edge::Top: {
            top = region;
            return;
        }
        case Edge::Bottom: {
            bottom = region;
            return;
        }
        case Edge::Left: {
            left = region;
            return;
        }
        case Edge::Right: {
            right = region;
            return;
        }
    }
}

QPoint RoutingRegion::RoutePath::from() const {
    const auto r = region->rect();
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

QPoint RoutingRegion::RoutePath::to() const {
    const auto r = region->rect();
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

const std::vector<RoutingRegion*> RoutingRegion::adjacentRegions() {
    return {top, bottom, left, right};
}

Edge RoutingRegion::adjacentRegion(const RoutingRegion* rr, bool& valid) const {
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

void RoutingRegion::registerRoute(Route* r, Direction d) {
    if (d == Direction::Horizontal) {
        horizontalRoutes.push_back(r);
    } else {
        verticalRoutes.push_back(r);
    }
}

bool RoutingRegion::operator==(const RoutingRegion& lhs) const {
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

void RoutingRegion::assignRoutes() {
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

}  // namespace eda
}  // namespace vsrtl

bool operator<(const QPoint& lhs, const QPoint& rhs) {
    return (lhs.x() < rhs.x()) || ((lhs.x() == rhs.x()) && (lhs.y() < rhs.y()));
}
