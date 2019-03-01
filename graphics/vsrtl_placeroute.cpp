#include "vsrtl_placeroute.h"
#include "vsrtl_component.h"
#include "vsrtl_componentgraphic.h"
#include "vsrtl_graphics_defines.h"

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

    for (const auto& c : stack)
        std::cout << c->getName() << std::endl;

    // Position components
    QPointF pos = QPointF(25, 25);  // Start a bit offset from the parent borders
    for (const auto& c : stack) {
        ComponentGraphic* g = static_cast<ComponentGraphic*>(c->getGraphic());
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

}  // namespace vsrtl
