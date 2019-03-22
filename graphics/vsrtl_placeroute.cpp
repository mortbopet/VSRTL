#include "vsrtl_placeroute.h"
#include "vsrtl_component.h"
#include "vsrtl_componentgraphic.h"
#include "vsrtl_graphics_defines.h"
#include "vsrtl_traversal_util.h"

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

std::map<ComponentGraphic*, QPointF> topologicalSortPlacement(const std::vector<ComponentGraphic*>& components) {
    std::map<ComponentGraphic*, QPointF> placements;
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
    QPointF pos = QPointF(25, 25);  // Start a bit offset from the parent borders
    for (const auto& c : stack) {
        auto* g = getGraphic<ComponentGraphic*>(c);
        placements[g] = pos;
        pos.rx() += g->boundingRect().width() + COMPONENT_COLUMN_MARGIN;
    }

    return placements;
}

std::map<ComponentGraphic*, QPointF> PlaceRoute::placeAndRoute(const std::vector<ComponentGraphic*>& components) const {
    switch (m_placementAlgorithm) {
        case PlaceAlg::TopologicalSort: {
            return topologicalSortPlacement(components);
        }
        default:
            Q_ASSERT(false);
    }
}

QList<RoutingRegion> defineRoutingRegions(const Placement& placement) {
    // Check that a valid placement was received (all components contained within the chip boundary)
    Q_ASSERT(placement.chipRect.contains(boundingRectOfRects<QRectF>(placement.components)));

    // Note: Even though the QRect's are in purely integer coordinates, doing things by floating points provides various
    // Qt utility functions
    QList<RoutingRegion> regions;
    const auto& chipBoundary = placement.chipRect;

    QList<QLineF> hz_bounding_lines, vt_bounding_lines, hz_region_lines, vt_region_lines;

    // create horizontal and vertical bounding rectangle lines
    for (const auto& r : placement.components) {
        hz_bounding_lines << QLineF(r.topLeft(), r.topRight()) << QLineF(r.bottomLeft(), r.bottomRight());
        vt_bounding_lines << QLineF(r.topLeft(), r.bottomLeft()) << QLineF(r.topRight(), r.bottomRight());
    }

    // Extrude horizontal lines
    for (const auto& line : hz_bounding_lines) {
        // Stretch line to chip boundary
        QLineF stretchedLine = QLineF({chipBoundary.left(), line.p1().y()}, {chipBoundary.right(), line.p1().y()});

        // Narrow line until no boundaries are met
        for (const auto& v_line : vt_bounding_lines) {
            QPointF intersectPoint;
            if (stretchedLine.intersect(v_line, &intersectPoint) == QLineF::BoundedIntersection) {
                // check for intersection with own bounding rectangle (QLineF::Intersect intersect when a line ends on
                // top of another line, which is the case for the components own bounding rectangles)
                if (intersectPoint == v_line.p1() || intersectPoint == v_line.p2())
                    continue;

                // Contract based on point closest to original line segment
                if ((intersectPoint - line.p1()).manhattanLength() < (intersectPoint - line.p2()).manhattanLength()) {
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
        QLineF stretchedLine = QLineF({line.p1().x(), chipBoundary.top()}, {line.p1().x(), chipBoundary.bottom()});

        // Narrow line until no boundaries are met
        for (const auto& h_line : hz_bounding_lines) {
            QPointF intersectPoint;
            if (stretchedLine.intersect(h_line, &intersectPoint) == QLineF::BoundedIntersection) {
                if (intersectPoint == h_line.p1() || intersectPoint == h_line.p2())
                    continue;

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
    hz_region_lines << QLineF(chipBoundary.topLeft(), chipBoundary.topRight())
                    << QLineF(chipBoundary.bottomLeft(), chipBoundary.bottomRight());
    vt_region_lines << QLineF(chipBoundary.topLeft(), chipBoundary.bottomLeft())
                    << QLineF(chipBoundary.topRight(), chipBoundary.bottomRight());

    // Sort bounding lines
    // Top to bottom
    std::sort(hz_region_lines.begin(), hz_region_lines.end(),
              [](const QLineF& a, const QLineF& b) { return a.p1().y() < b.p1().y(); });
    // Left to right
    std::sort(vt_region_lines.begin(), vt_region_lines.end(),
              [](const QLineF& a, const QLineF& b) { return a.p1().x() < b.p1().x(); });

    // Find intersections between horizontal and vertical region lines, and create corresponding routing regions.
    QPointF regionTopRight, regionBottomLeft, regionBottomRight, regionTopLeft;
    for (int hi = 1; hi < hz_region_lines.size(); hi++) {
        for (int vi = 1; vi < vt_region_lines.size(); vi++) {
            const auto& vt_region_line = vt_region_lines[vi];
            const auto& hz_region_line = hz_region_lines[hi];
            if (vt_region_line.intersect(hz_region_line, &regionBottomRight) == QLineF::BoundedIntersection) {
                // Intersection found (bottom right of region), a region may be created.
                // 1. Locate the point above the current intersection point (top right of region)
                for (int hi_rev = hi - 1; hi_rev >= 0; hi_rev--) {
                    if (hz_region_lines[hi_rev].intersect(vt_region_line, &regionTopRight) ==
                        QLineF::BoundedIntersection) {
                        break;
                    }
                }

                // 2. Locate the point to the left of the current intersection point (bottom right of region)
                for (int vi_rev = vi - 1; vi_rev >= 0; vi_rev--) {
                    if (vt_region_lines[vi_rev].intersect(hz_region_line, &regionBottomLeft) ==
                        QLineF::BoundedIntersection) {
                        break;
                    }
                }

                // Sanity check
                Q_ASSERT(!regionBottomLeft.isNull() && !regionTopRight.isNull());
                regionTopLeft = QPointF(regionBottomLeft.x(), regionTopRight.y());

                // Create the region
                regions << RoutingRegion(QRectF(regionTopLeft.toPoint(), regionBottomRight.toPoint()));
            }
        }
    }

    // Remove regions which are enclosing components
    for (int i = regions.size() - 1; i >= 0; i--) {
        auto res = std::find(placement.components.begin(), placement.components.end(), regions[i].r);
        if (res != placement.components.end()) {
            regions.removeAt(i);
        }
    }

    return regions;
}  // namespace vsrtl

}  // namespace vsrtl
