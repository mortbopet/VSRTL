#include "vsrtl_placeroute.h"
#include "core/vsrtl_component.h"
#include "core/vsrtl_traversal_util.h"
#include "graphics/vsrtl_componentgraphic.h"
#include "graphics/vsrtl_graphics_defines.h"
#include "graphics/vsrtl_portgraphic.h"

#include <QRect>
#include <cmath>
#include <deque>

namespace vsrtl {
namespace pr {

template <typename T>
void largestRunningSum(const T& c, unsigned int& i_max, int& sum_max) {
    int sum = c[0];
    sum_max = sum;
    i_max = 0;
    for (unsigned int i = 1; i < c.size(); i++) {
        const auto newSum = sum + c[i];
        if (newSum > sum) {
            i_max = i;
            sum_max = newSum;
        }
        sum = newSum;
    }
}

int KLdValue(Component* node, const std::set<Component*>& A, const std::set<Component*>& B) {
    // Assert that the node is only present in one of the sets
    Q_ASSERT((A.count(node) == 0) ^ (B.count(node) == 0));
    const auto& internalSet = A.count(node) > 0 ? A : B;
    const auto& externalSet = B.count(node) > 0 ? A : B;

    int I_cost = 0;
    int E_cost = 0;

    /* Get connected components of node.
       Each call returns a mapping of connected component and number of occurances of said component. Merge the two
       maps. */
    auto connectedComponents = node->getInputComponents();
    for (const auto& c : node->getOutputComponents()) {
        // If no entry exists for c.first in connectedComponents, the entry will be zero initialized, and the value from
        // OutputComponents will be added to this zero.
        connectedComponents[c.first] = connectedComponents[c.first] + c.second;
    }

    // Compute internal and external costs
    for (const auto& c : connectedComponents) {
        if (internalSet.count(c.first) > 0) {
            I_cost += c.second;
        } else if (externalSet.count(c.first) > 0) {
            E_cost += c.second;
        } else {
            // Ignore connections to components which are not considered part of the current partitioning run
        }
    }

    return E_cost - I_cost;
}

template <typename T>
void splitContainer(const T& in, T& A, T& B) {
    unsigned int i = 0;
    for (const auto& it : in) {
        if (i < (in.size() / 2)) {
            A.emplace(it);
        } else {
            B.emplace(it);
        }
        i++;
    }
}

PartitionedGraph KernighanLin(std::set<Component*>& graph) {
    Q_ASSERT(graph.size() > 1);
    // Create an initial balanced distribution
    std::set<Component*> A, B;
    splitContainer(graph, A, B);

    // Initially all nodes may be selected and the locked nodes of each set are empty
    std::set<Component*> A_locked, B_locked;

    int g_max = INT_MIN;
    do {
        // A maximum number of swaps has been performed if either of the locked sets are equal to half of the original
        // set size. Note that this adds support of when graph is an uneven number of nodes.
        if (A_locked.size() == graph.size() / 2) {
            break;
        }
        // Calculate D value for each node in A, B,
        std::map<Component*, int> D;
        for (const auto& c : graph) {
            D[c] = KLdValue(c, A, B);
        }

        // let gv, av, and bv be empty lists
        std::vector<Component*> av, bv;
        std::vector<int> gv;

        // The nodes to be considered in this pass are all nodes of A and B which have not previously been moved
        std::set<Component*> A_pass;
        std::set<Component*> B_pass;
        std::set_difference(A.begin(), A.end(), A_locked.begin(), A_locked.end(),
                            std::inserter(A_pass, A_pass.begin()));
        std::set_difference(B.begin(), B.end(), B_locked.begin(), B_locked.end(),
                            std::inserter(B_pass, B_pass.begin()));
        const size_t passSize = (A_pass.size() + B_pass.size());
        for (unsigned int i = 0; i < passSize / 2; i++) {
            // find a from A and b from B, such that g = D[a] + D[b] - 2*c(a, b) is maximal
            Component* a = nullptr;
            Component* b = nullptr;
            int g_pass_max = INT_MIN;
            for (const auto& a_it : A_pass) {
                for (const auto& b_it : B_pass) {
                    // Cost between two nodes is equal to the D-value between the two nodes.
                    const int g = D[a_it] + D[b_it] - 2 * KLdValue(a_it, {a_it}, {b_it});
                    if (g > g_pass_max) {
                        a = a_it;
                        b = b_it;
                        g_pass_max = g;
                    }
                }
            }
            Q_ASSERT(a != nullptr && b != nullptr);
            // a and b which maximizes g has been found. Add values to gv, av, bv
            gv.push_back(g_pass_max);
            av.push_back(a);
            bv.push_back(b);
            // remove a and b from further consideration in this pass.
            A_pass.erase(a);
            B_pass.erase(b);

            // Update D values for the elements of A_pass, B_pass
            for (const auto& c : A_pass) {
                D[c] = KLdValue(c, A_pass, B_pass);
            }
            for (const auto& c : A_pass) {
                D[c] = KLdValue(c, A_pass, B_pass);
            }
        }

        // find index i which maximizes g_max, the sum of gv[0],...,gv[i]
        unsigned int i_max;
        largestRunningSum(gv, i_max, g_max);
        if (g_max > 0) {
            // Permanently exchange av[0],av[1],...,av[i] with bv[0],bv[1],...,bv[i]
            for (size_t i = 0; i <= i_max; i++) {
                A.erase(av[i]);
                A.emplace(bv[i]);
                B.erase(bv[i]);
                B.emplace(av[i]);
                A_locked.emplace(bv[i]);
                B_locked.emplace(av[i]);
            }
        }
    } while (g_max > 0);

    // A min-cut partitioned graph can now be returned
    return {A, B};
}

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
        if (visited.find(cc.first) != visited.end() && !visited[cc.first]) {
            topologicalSortUtil(cc.first, visited, stack);
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
    auto pos = QPoint(CHIP_MARGIN, CHIP_MARGIN);  // Start a bit offset from the chip boundary borders
    for (const auto& c : stack) {
        auto* g = getGraphic<ComponentGraphic*>(c);
        g->setGridPos(pos);
        // Add 4 grid spaces between each component
        pos.rx() += g->adjustedMinGridRect(true, false).width() + 4;
    }
}

template <typename V, typename T>
struct BinTree {
    std::unique_ptr<T> a, b;
    V value;
};

enum class CutlineDirection { Alternating, Repeating };
// The cutline direction within a node determines whether the children in the node are conceptually placed
// left/right or up/down
/*Horizontal:
 *   ____
 *  |___|a
 *  |___|b
 *
 * vertical:
 *   _____
 * a|  |  |b
 *  |__|__|
 */

class PartitioningTree : public BinTree<Component*, PartitioningTree> {
public:
    Direction cutlinedir;
    QRect cachedRect;

    void place(const QPoint offset) {
        // Given an offset, propagates placement values down the tree structure, modifying offset according
        if (a || b) {
            Q_ASSERT(cachedRect.isValid());  // should have been calculated by a previous call to rect()
            // Internal node
            // Add this node's rect to the offset, and propagate the call further down the tree
            QPoint a_offset = offset;
            QPoint b_offset = offset;
            if (cutlinedir == Direction::Horizontal) {
                a_offset -= QPoint(0, a->rect().height() / 2);
                b_offset += QPoint(0, b->rect().height() / 2);
            } else if (cutlinedir == Direction::Vertical) {
                a_offset -= QPoint(a->rect().width() / 2, 0);
                b_offset += QPoint(b->rect().width() / 2, 0);
            }
            a->place(a_offset);
            b->place(b_offset);
        } else if (value) {
            // Leaf node
            // Place the component based on the offset received. The component should be centered at the offset point.
            // GridPos() refers to the top-left corner of the component
            auto* g = getGraphic<ComponentGraphic*>(value);
            const auto componentRect = g->adjustedMinGridRect(false, false);
            g->setGridPos(offset - QPoint(componentRect.width() / 2, componentRect.height() / 2));
        }
    }

    QRect rect() {
        // Check if rect has already been calculated. If so, return cached value
        if (cachedRect.isValid()) {
            return cachedRect;
        }
        // Calculate rect
        if (a || b) {
            // Internal node
            // Get node rectangle sizes of subnodes and join them together based on this nodes cutline direction
            const auto& a_rect = a->rect();
            const auto& b_rect = b->rect();
            int width, height;
            switch (cutlinedir) {
                case Direction::Horizontal: {
                    // For horizontal cuts, components will be placed above and below each other. Width will be the
                    // width of the largest component, height will be the total height of the two components
                    width = a_rect.width() > b_rect.width() ? a_rect.width() : b_rect.width();
                    height = a_rect.height() + b_rect.height();
                    break;
                }
                case Direction::Vertical: {
                    // For vertical cuts, components will be placed left and right of each other. Height will be the
                    // height of the tallest component. Width will be the total width of the two components
                    width = a_rect.width() + b_rect.width();
                    height = a_rect.height() > b_rect.height() ? a_rect.height() : b_rect.height();
                    break;
                }
            }
            cachedRect = QRect(0, 0, width, height);
        } else if (value) {
            // Leaf node. Here we calculate the optimal rect size for a given component.
            auto* g = getGraphic<ComponentGraphic*>(value);

            /** @todo: A better estimation of the required padding around a component based on its number of IO ports */
            auto rect = g->adjustedMinGridRect(true, false);
            const int widthPadding = static_cast<int>(value->getInputs().size() + value->getOutputs().size());
            const int heightPadding = static_cast<int>(widthPadding / 2);
            cachedRect = rect.adjusted(0, 0, widthPadding, heightPadding);
        }
        return cachedRect;
    }
};

void recursivePartitioning(PartitioningTree& node, std::set<Component*> components, CutlineDirection dir) {
    // This function will recurse through a set of components, splitting the components into a binary tree. Within a
    // node in the tree, the left and right subtrees represents sets minimum cut sets, generated via. the Kernighan-Lin
    // algorithm.
    // Functionally, the algorithm can be seen as splitting a rectangle of graph nodes into subsequently smaller
    // rectangles. Divide and conquer!
    /*        1a
     *    ___________
     *   |     |     |
     *2a |     |     |2b
     *   |_____|_____|
     *   |     |     |
     *2c |     |     |2d
     *   |_____|_____|
     *        1b
     * And so forth, until the full binary tree has been resolved.
     */
    node.a = std::make_unique<PartitioningTree>();
    node.b = std::make_unique<PartitioningTree>();

    // Assign cutline direction to child nodes
    Direction childrenCutlineDir;
    if (dir == CutlineDirection::Alternating) {
        childrenCutlineDir = node.cutlinedir == Direction::Horizontal ? Direction::Vertical : Direction::Horizontal;
    } else if (dir == CutlineDirection::Repeating) {
        // Unimplemnented
        Q_ASSERT(false);
    }
    node.a->cutlinedir = childrenCutlineDir;
    node.b->cutlinedir = childrenCutlineDir;
    if (components.size() <= 2) {
        // Leaf nodes, assign components in the tree
        node.a->value = *components.begin();
        if (components.size() == 2) {
            node.b->value = *components.rbegin();
        }
    } else {
        // partition the graph into two disjoint sets
        auto partitionedGraph = KernighanLin(components);
        // Run the cycle util with new nodes in the tree for each side of the partitioned graph
        recursivePartitioning(*node.a, partitionedGraph.first, dir);
        recursivePartitioning(*node.b, partitionedGraph.second, dir);
    }
}

void MinCutPlacement(const std::vector<ComponentGraphic*>& components) {
    /**
      Min cut placement:
      1. Use a partitioning algorithm to divide the netlist as well as the layout region in progressively smaller
      sub-regions and sub-netlists
      2. Each sub-region is assigned to a part of the sub-netlist.
      3. The layout is divided in binary,steps and may be explored as a binary tree
    */
    // Create a set of components
    std::set<Component*> c_set;
    for (const auto& c : components) {
        c_set.emplace(c->getComponent());
    }

    // recursively partition the graph and routing regions. Initial cut is determined to be a vertical cut
    PartitioningTree rootPartitionNode;
    rootPartitionNode.cutlinedir = Direction::Vertical;
    recursivePartitioning(rootPartitionNode, c_set, CutlineDirection::Alternating);

    // With the circuit partitioned, call rect() on the top node, to propagate rect calculation and caching through the
    // tree
    rootPartitionNode.rect();

    // Assign the offset where the placement algorithm will start at.
    // This offset will be the center between the two initial partitions + a small offset in the +x, +y direction to
    // move the entire circuit away from the edge of the chip, making space for wires to be routed around the edge of
    // the chip area.
    /*
     * __________
     * | a |  b  |
     * |   |     |
     * |   x     |
     * |   |     |
     * |___|_____|
     */
    const QRect leftRect = rootPartitionNode.a->rect();
    const int padding = 2;
    QPoint offset(leftRect.right(), leftRect.center().y());
    offset += QPoint(padding, padding);
    rootPartitionNode.place(offset);
}

namespace {
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

        m_orientation = p1.x() == p2.x() ? Direction::Vertical : Direction::Horizontal;
    }

    const QPoint& p1() const { return m_p1; }
    const QPoint& p2() const { return m_p2; }
    void setP1(const QPoint& p) { m_p1 = p; }
    void setP2(const QPoint& p) { m_p2 = p; }
    Direction orientation() const { return m_orientation; }
    bool intersect(const Line& other, QPoint& p, IntersectType type) const {
        Q_ASSERT(orientation() != other.orientation());
        const Line *hz, *vt;
        if (m_orientation == Direction::Horizontal) {
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
    Direction m_orientation;
};

static inline Line getEdge(const QRect& rect, Edge e) {
    switch (e) {
        case Edge::Top: {
            return Line(rect.topLeft(), rect.topRight());
        }
        case Edge::Bottom: {
            return Line(rect.bottomLeft(), rect.bottomRight());
        }
        case Edge::Left: {
            return Line(rect.topLeft(), rect.bottomLeft());
        }
        case Edge::Right: {
            return Line(rect.topRight(), rect.bottomRight());
        }
    }
}

}  // namespace

/**
 * @brief The RegionMap class
 * Data structure for retrieving the region group which envelops the provided index.
 */
class RegionMap {
public:
    RegionMap(const RoutingRegions& regions) {
        // Regions will be mapped to their lower-right corner in terms of indexing operations.
        // This is given the user of std::map::lower_bound to determine the map index
        for (const auto& region : regions) {
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
    // Indexable region map
    std::map<int, std::map<int, RoutingRegion*>> regionMap;
};

NetlistPtr createNetlist(Placement& placement, const RegionMap& regionMap) {
    auto netlist = std::make_unique<Netlist>();
    for (const auto& routingComponent : placement.components) {
        for (const auto& outputPort : routingComponent.componentGraphic->outputPorts()) {
            // Note: terminal position currently is fixed to right => output, left => input
            auto net = std::make_unique<Net>();
            NetNode source;
            source.componentGraphic = routingComponent.componentGraphic;
            source.edgeIndex = outputPort->gridIndex();
            source.edgePos = Edge::Right;
            source.port = outputPort;

            // Get source port grid position
            QPoint portPos = routingComponent.topRight();
            portPos.ry() += source.edgeIndex;
            source.region = regionMap.lookup(portPos, Edge::Right);
            for (const auto& sinkPort : outputPort->getPort()->getOutputPorts()) {
                NetNode sink;
                sink.port = getGraphic<PortGraphic*>(sinkPort);
                sink.componentGraphic = getGraphic<ComponentGraphic*>(sinkPort->getParent());
                // Lookup routing component for sink component graphic
                auto rc_i =
                    std::find_if(placement.components.begin(), placement.components.end(),
                                 [&sink](const auto& rc) { return rc.componentGraphic == sink.componentGraphic; });
                Q_ASSERT(rc_i != placement.components.end());
                sink.edgeIndex = sink.port->gridIndex();
                sink.edgePos = Edge::Left;

                // Get sink port grid position
                portPos = rc_i->topLeft();
                portPos.ry() += sink.edgeIndex;
                sink.region = regionMap.lookup(portPos, Edge::Left);
                net->push_back(std::make_unique<Route>(source, sink));
            }
            netlist->push_back(std::move(net));
        }
    }
    return netlist;
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

void RegionGroup::connectRegions() {
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

RoutingRegions createConnectivityGraph(Placement& placement) {
    // Check that a valid placement was received (all components contained within the chip boundary)
    Q_ASSERT(placement.chipRect.contains(boundingRectOfRects(placement.components)));
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
                // Note: (1,1) is subtracted from the bottom right corner to transform the coordinates into the QRect
                // expected format of the bottom-right corner
                QRect newRegionRect = QRect(regionTopLeft, regionBottomRight);

                if (std::find(placement.components.begin(), placement.components.end(), newRegionRect) ==
                    placement.components.end()) {
                    if (std::find_if(regions.begin(), regions.end(), [&newRegionRect](const auto& p) {
                            return p.get()->rect() == newRegionRect;
                        }) == regions.end())

                        // This is a new routing region
                        regions.push_back(std::make_unique<RoutingRegion>(newRegionRect));
                    RoutingRegion* newRegion = regions.rbegin()->get();

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
        Q_ASSERT(regionGroups.count(rc.topLeft()));
        Q_ASSERT(regionGroups.count(rc.topRight()));
        Q_ASSERT(regionGroups.count(rc.bottomRight()));
        Q_ASSERT(regionGroups.count(rc.bottomLeft()));
        rc.topRegion = regionGroups[rc.topLeft()].topright;
        rc.leftRegion = regionGroups[rc.topLeft()].bottomleft;
        rc.rightRegion = regionGroups[rc.topRight()].bottomright;
        rc.bottomRegion = regionGroups[rc.bottomLeft()].bottomright;
    }

    return regions;
}

const std::vector<RoutingRegion*> RoutingRegion::adjacentRegions() {
    return {top, bottom, left, right};
}

void RoutingRegion::registerRoute(Route* r, Direction d) {
    if (d == Direction::Horizontal) {
        horizontalRoutes.push_back(r);
    } else {
        verticalRoutes.push_back(r);
    }
}

void RoutingRegion::assignRoutes() {
    const float hz_diff = static_cast<float>(h_cap) / (horizontalRoutes.size() + 1);
    const float vt_diff = static_cast<float>(v_cap) / (verticalRoutes.size() + 1);

    float hz_pos = hz_diff;
    float vt_pos = vt_diff;
    for (const auto& route : horizontalRoutes) {
        assignedRoutes[route] = {Direction::Horizontal, hz_pos};
        hz_pos += hz_diff;
    }
    for (const auto& route : verticalRoutes) {
        assignedRoutes[route] = {Direction::Horizontal, vt_pos};
        vt_pos += vt_diff;
    }
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

QRect qrectToGridRect(const QRect& rect) {
    QPoint bottomRight = rect.topLeft();
    bottomRight += QPoint(rect.width(), rect.height());
    return QRect(rect.topLeft(), bottomRight);
}

template <typename K, typename V>
V getDef(const std::map<K, V>& m, const K& key, const V& defval) {
    typename std::map<K, V>::const_iterator it = m.find(key);
    if (it == m.end()) {
        return defval;
    } else {
        return it->second;
    }
}

int heuristicCost(RoutingRegion* start, RoutingRegion* goal) {
    // The heuristic cost estimate is the manhattan distance between the center of the two routing regions
    return (goal->rect().center() - start->rect().center()).manhattanLength();
}

std::vector<RoutingRegion*> reconstructPath(Route* route, std::map<RoutingRegion*, RoutingRegion*> cameFromMap,
                                            RoutingRegion* current) {
    std::vector<RoutingRegion*> totalPath{current};

    while (cameFromMap.count(current) > 0) {
        auto cameFrom = cameFromMap[current];
        // Based on the difference between the centers of the two routing regions, figure out if the move was
        // horizontally or vertically.
        // In the given routing region, assign a track to the Route
        QPoint diff = cameFrom->rect().center() - current->rect().center();
        Q_ASSERT(diff.x() == 0 || diff.y() == 0);
        cameFrom->registerRoute(route, diff.x() == 0 ? Direction::Vertical : Direction::Horizontal);

        current = cameFrom;
        totalPath.insert(totalPath.begin(), current);
    }
    return totalPath;
}

void findRoute(std::unique_ptr<Route>& route) {
    // Blatantly copied from https://en.wikipedia.org/wiki/A*_search_algorithm
    // Precondition: start- and stop regions must have their horizontal and vertical capacities pre-decremented for the
    // given number of terminals within them

    RoutingRegion* start = route->start.region;
    RoutingRegion* goal = route->end.region;

    // The set of nodes already evaluated
    std::set<RoutingRegion*> closedSet;

    // The set of currently discovered nodes that are not evaluated yet.
    // Initially, only the start node is known.
    std::set<RoutingRegion*> openSet{start};

    // For each node, which node it can most efficiently be reached from.
    // If a node can be reached from many nodes, cameFrom will eventually contain the
    // most efficient previous step.
    std::map<RoutingRegion*, RoutingRegion*> cameFrom;

    // For each node, the cost of getting from the start node to that node.
    std::map<RoutingRegion*, int> gScore;

    // For each node, the total cost of getting from the start node to the goal
    // by passing by that node. That value is partly known, partly heuristic.
    std::map<RoutingRegion*, int> fScore;

    // The cost of going from start to start is zero.
    gScore[start] = 0;

    // For the first node, that value is completely heuristic.
    fScore[start] = heuristicCost(start, goal);

    RoutingRegion* current = nullptr;
    while (!openSet.empty()) {
        // Find node in openSet with the lowest fScore value
        int lowestScore = INT_MAX;
        for (const auto& node : openSet) {
            if (getDef(fScore, node, INT_MAX) < lowestScore) {
                current = node;
                lowestScore = fScore[node];
            }
        }

        if (current == goal) {
            route->path = reconstructPath(route.get(), cameFrom, current);
            return;
        }

        openSet.erase(current);
        closedSet.emplace(current);

        for (const auto& neighbour : current->adjacentRegions()) {
            if (neighbour == nullptr) {
                continue;
            }
            if (closedSet.count(neighbour) > 0) {
                // Ignore the neighbor which is already evaluated.
                continue;
            }

            // The distance from start to a neighbor
            int tentative_gScore = getDef(gScore, current, INT_MAX) + heuristicCost(current, neighbour);

            if (openSet.count(neighbour) == 0) {
                // Discovered a new node
                openSet.emplace(neighbour);
            } else if (tentative_gScore >= getDef(gScore, neighbour, INT_MAX)) {
                continue;
            }

            // This path is the best until now. Record it!
            cameFrom[neighbour] = current;
            gScore[neighbour] = tentative_gScore;
            fScore[neighbour] = getDef(gScore, neighbour, INT_MAX) + heuristicCost(neighbour, goal);
        }
    }
    Q_ASSERT(false);
}

void PlaceRoute::placeAndRoute(const std::vector<ComponentGraphic*>& components, RoutingRegions& regions) {
    // Placement
    switch (m_placementAlgorithm) {
        case PlaceAlg::Topological1D: {
            topologicalSortPlacement(components);
            break;
        }
        case PlaceAlg::MinCut: {
            MinCutPlacement(components);
            break;
        }
    }
    // Connectivity graph: Transform current components into a placement format suitable for creating the connectivity
    // graph
    Placement placement;
    for (const auto& c : components) {
        RoutingComponent rc;
        rc = (c->adjustedMinGridRect(true, true));
        rc.componentGraphic = c;
        placement.components << rc;
    }
    placement.chipRect = boundingRectOfRects(placement.components);
    // Add margins to chip rect to allow routing on right- and bottom borders
    placement.chipRect.adjust(0, 0, CHIP_MARGIN, CHIP_MARGIN);
    auto cGraph = createConnectivityGraph(placement);

    // Indexable region map
    const auto regionMap = RegionMap(cGraph);

    /* ======================= ROUTING ======================= */
    auto netlist = createNetlist(placement, regionMap);

    // Route via. a* search between start- and stop nodes, using the available routing regions
    for (auto& net : *netlist) {
        if (net->size() == 0) {
            // Skip empty nets
            continue;
        }

        // Find a route to each start-stop pair in the net
        for (auto& route : *net) {
            findRoute(route);
        }
        // Move net pointer ownership to a start port of the net (All start ports are equal within the net)
        (*net)[0]->start.port->setNet(net);
    }

    // During findRoute, all routes have registered to their routing regions. With knowledge of how many routes occupy
    // each routing region, a route is assigned a lane within the routing region
    for (const auto& region : cGraph) {
        region->assignRoutes();
    }

    regions = std::move(cGraph);
}

}  // namespace pr
}  // namespace vsrtl

// less-than operator for QPointF, required for storing QPointF as index in a std::map
bool operator<(const QPoint& p1, const QPoint& p2) {
    if (p1.x() == p2.x()) {
        return p1.y() < p2.y();
    }
    return p1.x() < p2.x();
}
