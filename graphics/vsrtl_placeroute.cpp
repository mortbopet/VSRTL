#include "vsrtl_placeroute.h"
#include "vsrtl_component.h"
#include "vsrtl_componentgraphic.h"
#include "vsrtl_graphics_defines.h"
#include "vsrtl_portgraphic.h"
#include "vsrtl_traversal_util.h"

#include <QRect>
#include <cmath>
#include <deque>

namespace vsrtl {

/**
 * @brief topologicalSortUtil
 * Subcomponent ordering is based on a topological sort algorithm.
 * This algorithm is applicable for DAG's (Directed Acyclical Graph's). Naturally, digital circuits does not fulfull the
 * requirement for being a DAG. However, if Registers are seen as having either their input or output disregarded as an
 * edge in a DAG, a DAG can be created from a digital circuit, if only outputs are considered.
 * Having a topological sorting of the components, rows- and columns can
 * be generated for each depth in the topological sort, wherein the components are finally layed out.
 * @param parent
 */
void topologicalSortUtil(Component* c, std::map<Component*, bool>& visited, std::deque<Component*>& stack) {
    visited[c] = true;

    for (const auto& cc : c->getOutputComponents()) {
        // The find call ensures that the output components are inside this subcomponent. OutputComponents are "parent"
        // agnostic, and has no notion of enclosing components.
        if (visited.find(cc) != visited.end() && !visited[cc]) {
            topologicalSortUtil(cc, visited, stack);
        }
    }
    stack.push_front(c);
}

void topologicalSortPlacement(const std::vector<ComponentGraphic*>& components) {
    std::map<Component*, bool> visited;
    std::deque<Component*> stack;

    for (const auto& cpt : components)
        visited[cpt->getComponent()] = false;

    for (const auto& c : visited) {
        if (!c.second) {
            topologicalSortUtil(c.first, visited, stack);
        }
    }

    // Position components
    auto pos = QPoint(4, 4);  // Start a bit offset from the chip boundary borders
    for (const auto& c : stack) {
        auto* g = getGraphic<ComponentGraphic*>(c);
        g->setGridPos(pos);
        // Add 4 grid spaces between each component
        pos.rx() += g->adjustedMinGridRect(true).width() + 4;
    }
}

namespace {
struct RegionGroup {
    RoutingRegion* topleft = nullptr;
    RoutingRegion* topright = nullptr;
    RoutingRegion* bottomleft = nullptr;
    RoutingRegion* bottomright = nullptr;
};

enum Orientation { Horizontal, Vertical };
enum IntersectType { Cross, OnEdge };

/**
 * @brief The Line class
 * Simple orthogonal line class with integer coordinates. Similar to QLine, however, this does not carry the strange
 * "historical" artifacts which are present in QLine.
 */
class Line {
public:
    Line(QPoint p1, QPoint p2) {
        // Assert that the line is orthogonal to one of the grid axis
        Q_ASSERT(p1.x() == p2.x() || p1.y() == p2.y());
        m_p1 = p1;
        m_p2 = p2;

        m_orientation = p1.x() == p2.x() ? Orientation::Vertical : Orientation::Horizontal;
    }

    const QPoint& p1() const { return m_p1; }
    const QPoint& p2() const { return m_p2; }
    void setP1(const QPoint& p) { m_p1 = p; }
    void setP2(const QPoint& p) { m_p2 = p; }
    Orientation orientation() const { return m_orientation; }
    bool intersect(const Line& other, QPoint& p, IntersectType type) const {
        Q_ASSERT(orientation() != other.orientation());
        const Line *hz, *vt;
        if (m_orientation == Orientation::Horizontal) {
            hz = this;
            vt = &other;
        } else {
            hz = &other;
            vt = this;
        }

        // Assert that the lines are orthogonal
        Q_ASSERT(hz->p1().y() == hz->p2().y());
        Q_ASSERT(vt->p2().x() == vt->p2().x());

        bool hz_intersect, vt_intersect;
        if (type == IntersectType::Cross) {
            // Lines must cross before an intersection is registered
            hz_intersect = (hz->p1().x() < vt->p1().x()) && (vt->p1().x() < hz->p2().x());
            vt_intersect = (vt->p1().y() < hz->p1().y()) && (hz->p1().y() < vt->p2().y());
        } else {
            // Lines may terminate on top of another for an intersection to register
            hz_intersect = (hz->p1().x() <= vt->p1().x()) && (vt->p1().x() <= hz->p2().x());
            vt_intersect = (vt->p1().y() <= hz->p1().y()) && (hz->p1().y() <= vt->p2().y());
        }

        if (hz_intersect && vt_intersect) {
            p = QPoint(vt->p1().x(), hz->p1().y());
            return true;
        }
        p = QPoint();
        return false;
    }

    bool operator==(const Line& rhs) const { return m_p1 == rhs.m_p1 && m_p2 == rhs.m_p2; }

private:
    QPoint m_p1;
    QPoint m_p2;
    Orientation m_orientation;
};

static inline Line getEdge(const QRect& rect, Edge e) {
    // Qt "For historical reasons" return points which are offset by 1 for all rect points other than topleft. To ensure
    // control over this, we manually derive edge points, https://doc.qt.io/Qt-5/qrect.html#bottomRight
    const auto bottomLeft = rect.topLeft() + QPoint(0, rect.height());
    const auto topRight = rect.topLeft() + QPoint(rect.width(), 0);
    const auto bottomRight = bottomLeft + QPoint(rect.width(), 0);

    switch (e) {
        case Edge::Top: {
            return Line(rect.topLeft(), topRight);
        }
        case Edge::Bottom: {
            return Line(bottomLeft, bottomRight);
        }
        case Edge::Left: {
            return Line(rect.topLeft(), bottomLeft);
        }
        case Edge::Right: {
            return Line(topRight, bottomRight);
        }
    }
}

}  // namespace

Netlist createNetlist(const std::vector<ComponentGraphic*>& components) {
    Netlist netlist;
    for (const auto& c : components) {
        for (const auto& outputPort : c->outputPorts()) {
            Net net;
            NetNode source;
            source.edgeIndex = outputPort->gridIndex();
            source.edgePos = Edge::Right;
            source.port = outputPort;
            net.push_back(source);
            for (const auto& sinkPort : outputPort->getPort()->getOutputPorts()) {
                NetNode sink;
                sink.port = getGraphic<PortGraphic*>(sinkPort);
                sink.edgeIndex = sink.port->gridIndex();
                sink.edgePos = Edge::Left;
                net.push_back(sink);
            }
            netlist.push_back(net);
        }
    }
    return netlist;
}

std::vector<std::unique_ptr<RoutingRegion>> createConnectivityGraph(const Placement& placement) {
    // Check that a valid placement was received (all components contained within the chip boundary)
    Q_ASSERT(placement.chipRect.contains(boundingRectOfRects<QRect>(placement.components)));
    Q_ASSERT(placement.chipRect.topLeft() == QPoint(0, 0));

    std::vector<std::unique_ptr<RoutingRegion>> regions;
    const auto& chipRect = placement.chipRect;

    QList<Line> hz_bounding_lines, vt_bounding_lines, hz_region_lines, vt_region_lines;

    // Get horizontal and vertical bounding rectangle lines for all components on chip
    for (const auto& r : placement.components) {
        hz_bounding_lines << getEdge(r, Edge::Top) << getEdge(r, Edge::Bottom);
        vt_bounding_lines << getEdge(r, Edge::Left) << getEdge(r, Edge::Right);
    }

    // ============== Component edge extrusion =================
    // This step extrudes the horizontal and vertical bounding rectangle lines for each component on the chip. The
    // extrusion will extend the line in each direction, until an obstacle is met (chip edges or other components)

    // Extrude horizontal lines
    for (const auto& h_line : hz_bounding_lines) {
        // Stretch line to chip boundary
        Line stretchedLine = Line({chipRect.left(), h_line.p1().y()}, {chipRect.width(), h_line.p1().y()});

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
        Line stretchedLine = Line({line.p1().x(), chipRect.top()}, {line.p1().x(), chipRect.height()});

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

    // Add chip boundaries to region lines
    hz_region_lines << getEdge(chipRect, Edge::Top) << getEdge(chipRect, Edge::Bottom);
    vt_region_lines << getEdge(chipRect, Edge::Left) << getEdge(chipRect, Edge::Right);

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
    QPoint regionTopRight, regionBottomLeft, regionBottomRight, regionTopLeft;
    QPoint regionBottom, regionTop;
    Line* topHzLine;
    for (int hi = 1; hi < hz_region_lines.size(); hi++) {
        for (int vi = 1; vi < vt_region_lines.size(); vi++) {
            if (hi == 3 && vi == 2) {
                volatile int a;
            }
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

                // Set top right and left coordinates
                regionTopRight = QPoint(regionBottomRight.x(), regionTop.y());
                regionTopLeft = QPoint(regionBottomLeft.x(), regionTop.y());

                // Sanity check
                Q_ASSERT(!regionBottomLeft.isNull() && !regionTopRight.isNull());

                // Check whether the region is enclosing a component.
                // Note: (1,1) is subtracted from the bottom right corner to transform the coordinates into the QRect
                // expected format of the bottom-right corner
                QRect newRegionRect = QRect(regionTopLeft, regionBottomRight - QPoint(1, 1));

                if (std::find(placement.components.begin(), placement.components.end(), newRegionRect) ==
                    placement.components.end()) {
                    if (std::find_if(regions.begin(), regions.end(), [&newRegionRect](const auto& p) {
                            return p.get()->r == newRegionRect;
                        }) == regions.end())

                        // This is a new routing region
                        regions.push_back(std::make_unique<RoutingRegion>(newRegionRect));
                    RoutingRegion* newRegion = regions.rbegin()->get();

                    // Add region to regionGroups
                    regionGroups[regionTopLeft].bottomright = newRegion;
                    regionGroups[regionBottomLeft].topright = newRegion;
                    regionGroups[regionTopRight].bottomleft = newRegion;
                    regionGroups[regionBottomRight].topleft = newRegion;
                }
            }
        }
    }

    // =================== Connectivity graph connection ========================== //
    for (const auto& iter : regionGroups) {
        const RegionGroup& group = iter.second;
        if (group.topleft != nullptr) {
            group.topleft->bottom = group.bottomleft;
            group.topleft->right = group.topright;
        }

        if (group.topright != nullptr) {
            group.topright->left = group.topleft;
            group.topright->bottom = group.bottomright;
        }
        if (group.bottomleft != nullptr) {
            group.bottomleft->top = group.topleft;
            group.bottomleft->right = group.bottomright;
        }
        if (group.bottomright != nullptr) {
            group.bottomright->left = group.bottomleft;
            group.bottomright->top = group.topright;
        }
    }

    return regions;
}

void PlaceRoute::placeAndRoute(const std::vector<ComponentGraphic*>& components) const {
    auto netlist = createNetlist(components);
    // Placement
    switch (m_placementAlgorithm) {
        case PlaceAlg::TopologicalSort: {
            topologicalSortPlacement(components);
            break;
        }
        default:
            Q_ASSERT(false);
    }

    // Place graphical components based on new position of grid components
    for (const auto& c : components) {
        const QPoint gridPos = c->adjustedMinGridRect(true).topLeft();
        c->setPos(gridPos * GRID_SIZE);
    }

    // Connectivity graph: Transform current components into a placement format suitable for creating the connectivity
    // graph
    Placement placement;
    for (const auto& c : components) {
        placement.components << c->adjustedMinGridRect(true);
    }
    placement.chipRect = boundingRectOfRects(placement.components);
    auto cGraph = createConnectivityGraph(placement);

    // Placement
}

}  // namespace vsrtl

// less-than operator for QPointF, required for storing QPointF as index in a std::map
bool operator<(const QPointF& p1, const QPointF& p2) {
    if (p1.x() == p2.x()) {
        return p1.y() < p2.y();
    }
    return p1.x() < p2.x();
}
