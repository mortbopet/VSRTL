#include "eda/algorithms/vsrtl_kernighanlin.h"
#include "eda/vsrtl_tilegraph.h"
#include "vsrtl_gridcomponent.h"
#include "vsrtl_placement.h"

namespace vsrtl {
namespace eda {

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

class PartitioningTree : public BinTree<SimComponent*, PartitioningTree> {
public:
    explicit PartitioningTree(PhysicalPlacement& _placement) : BinTree(), placement(_placement) {}
    Orientation cutlinedir = Orientation::Horizontal;
    QRect cachedRect;
    PhysicalPlacement& placement;

    void place(const QPoint offset) {
        // Given an offset, propagates placement values down the tree structure, modifying offset according
        if (a || b) {
            Q_ASSERT(cachedRect.isValid());  // should have been calculated by a previous call to rect()
            // Internal node
            // Add this node's rect to the offset, and propagate the call further down the tree
            QPoint a_offset = offset;
            QPoint b_offset = offset;
            if (cutlinedir == Orientation::Horizontal) {
                a_offset -= QPoint(0, a->rect().height() / 2);
                b_offset += QPoint(0, b->rect().height() / 2);
            } else if (cutlinedir == Orientation::Vertical) {
                a_offset -= QPoint(a->rect().width() / 2, 0);
                b_offset += QPoint(b->rect().width() / 2, 0);
            }
            a->place(a_offset);
            b->place(b_offset);
        } else if (value) {
            // Leaf node
            // Place the component based on the offset received. The component should be centered at the offset point.
            // GridPos() refers to the top-left corner of the component
            auto* gridComponent = value->getGraphic<GridComponent>();
            Q_ASSERT(gridComponent);
            const auto w = gridComponent->getCurrentComponentRect().width();
            const auto h = gridComponent->getCurrentComponentRect().height();
            const QPoint pos = (offset - QPoint(w / 2, h / 2));
            ComponentTile rc(gridComponent);
            rc.setPos(pos);
            placement.addComponent(std::make_shared<ComponentTile>(rc));
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
                case Orientation::Horizontal: {
                    // For horizontal cuts, components will be placed above and below each other. Width will be the
                    // width of the largest component, height will be the total height of the two components
                    width = a_rect.width() > b_rect.width() ? a_rect.width() : b_rect.width();
                    height = a_rect.height() + b_rect.height();
                    break;
                }
                case Orientation::Vertical: {
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
            auto* gridComponent = value->getGraphic<GridComponent>();
            Q_ASSERT(gridComponent);

            /** @todo: A better estimation of the required padding around a component based on its number of IO ports */
            auto rect = gridComponent->getCurrentComponentRect();
            // const int widthPadding = static_cast<int>(value->getInputs().size() + value->getOutputs().size());
            // const int heightPadding = static_cast<int>(widthPadding / 2);
            // cachedRect = rect.adjusted(0, 0, widthPadding, heightPadding);
            cachedRect = rect;
        }
        return cachedRect;
    }
};

void recursivePartitioning(PartitioningTree& node, const std::set<SimComponent*>& components, CutlineDirection dir) {
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
    node.a = std::make_unique<PartitioningTree>(node.placement);
    node.b = std::make_unique<PartitioningTree>(node.placement);

    // Assign cutline direction to child nodes
    Orientation childrenCutlineDir = Orientation::Horizontal;
    if (dir == CutlineDirection::Alternating) {
        childrenCutlineDir =
            node.cutlinedir == Orientation::Horizontal ? Orientation::Vertical : Orientation::Horizontal;
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
        auto partitionedGraph = KernighanLin(components, &SimComponent::getConnectedComponents<SimComponent>);
        // Run the cycle util with new nodes in the tree for each side of the partitioned graph
        recursivePartitioning(*node.a, partitionedGraph.first, dir);
        recursivePartitioning(*node.b, partitionedGraph.second, dir);
    }
}

std::shared_ptr<Placement> MinCutPlacement(const std::vector<GridComponent*>& components) {
    /**
      Min cut placement:
      1. Use a partitioning algorithm to divide the netlist as well as the layout region in progressively smaller
      sub-regions and sub-netlists
      2. Each sub-region is assigned to a part of the sub-netlist.
      3. The layout is divided in binary,steps and may be explored as a binary tree
    */
    // Create a set of components
    std::set<SimComponent*> c_set;
    for (const auto& c : components) {
        c_set.emplace(c->getComponent());
    }

    auto placement = std::make_shared<PhysicalPlacement>();

    // recursively partition the graph and routing regions. Initial cut is determined to be a vertical cut
    PartitioningTree rootPartitionNode(*static_cast<PhysicalPlacement*>(placement.get()));
    rootPartitionNode.cutlinedir = Orientation::Vertical;
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

    return placement;
}

}  // namespace eda
}  // namespace vsrtl
