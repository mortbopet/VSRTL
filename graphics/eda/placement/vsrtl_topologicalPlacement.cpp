#include "vsrtl_gridcomponent.h"
#include "vsrtl_placement.h"

#include <deque>

namespace vsrtl {
namespace eda {

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
void topologicalSortUtil(SimComponent* c, std::map<SimComponent*, bool>& visited, std::deque<SimComponent*>& stack) {
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

/// @TODO: rewrite this to a generic algorithm and move to the algorithm library.
std::deque<SimComponent*> topologicalSort(const std::vector<GridComponent*>& components) {
    std::map<SimComponent*, bool> visited;
    std::deque<SimComponent*> stack;

    for (const auto& cpt : components)
        visited[cpt->getComponent()] = false;

    for (const auto& c : visited) {
        if (!c.second) {
            topologicalSortUtil(c.first, visited, stack);
        }
    }
    return stack;
}

std::shared_ptr<Placement> topologicalSortPlacement(const std::vector<GridComponent*>& components) {
    std::shared_ptr<Placement> placements;
    std::deque<SimComponent*> sortedComponents = topologicalSort(components);

    // Position components
    QPoint pos = QPoint(SUBCOMPONENT_INDENT, SUBCOMPONENT_INDENT);  // Start a bit offset from the parent borders
    for (const auto& c : sortedComponents) {
        auto* g = c->getGraphic<GridComponent>();
        auto routable = RoutingComponent(g);
        routable.pos = pos;
        placements->components.push_back(std::make_shared<RoutingComponent>(routable));
        pos.rx() += g->getCurrentComponentRect().width() + COMPONENT_COLUMN_MARGIN;
    }

    return placements;
}

std::map<int, std::set<SimComponent*>> ASAPSchedule(const std::vector<GridComponent*>& components) {
    std::deque<SimComponent*> sortedComponents = topologicalSort(components);
    std::map<int, std::set<SimComponent*>> schedule;
    std::map<SimComponent*, int> componentToDepth;

    // 1. assign a depth to the components based on their depth in the topological sort, disregarding output edges
    for (const auto& c : sortedComponents) {
        int lastPredLayer = -1;
        for (const auto& inC : c->getInputComponents()) {
            if (componentToDepth.count(inC) != 0) {
                int predDepth = componentToDepth.at(inC);
                lastPredLayer = lastPredLayer < predDepth ? predDepth : lastPredLayer;
            }
        }
        componentToDepth[c] = lastPredLayer + 1;
    }

    // 2. Create a map between depths and components
    for (const auto& c : componentToDepth) {
        schedule[c.second].insert(c.first);
    }

    return schedule;
}

std::shared_ptr<Placement> ASAPPlacement(const std::vector<GridComponent*>& components) {
    auto placements = std::make_shared<Placement>();
    const auto asapSchedule = ASAPSchedule(components);

    // 1. create a width of each column
    std::map<int, int> columnWidths;
    for (const auto& iter : asapSchedule) {
        int maxWidth = 0;
        for (const auto& c : iter.second) {
            int width = c->getGraphic<GridComponent>()->getCurrentComponentRect().width();
            maxWidth = maxWidth < width ? width : maxWidth;
        }
        columnWidths[iter.first] = maxWidth;
    }

    // Position components
    // Start a bit offset from the parent borders
    const QPoint start{SUBCOMPONENT_INDENT, SUBCOMPONENT_INDENT};
    int x = start.x();
    int y = start.y();
    for (const auto& iter : asapSchedule) {
        for (const auto& c : iter.second) {
            auto* g = c->getGraphic<GridComponent>();
            auto routable = RoutingComponent(g);
            routable.pos = QPoint(x, y);
            placements->components.push_back(std::make_shared<RoutingComponent>(routable));
            y += g->getCurrentComponentRect().height() + COMPONENT_COLUMN_MARGIN;
        }
        x += columnWidths[iter.first] + 2 * COMPONENT_COLUMN_MARGIN;
        y = start.y();
    }

    return placements;
}

}  // namespace eda
}  // namespace vsrtl