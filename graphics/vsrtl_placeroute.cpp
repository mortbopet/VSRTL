#include "vsrtl_placeroute.h"
#include "vsrtl_component.h"
#include "vsrtl_componentgraphic.h"
#include "vsrtl_graphics_defines.h"

#include <deque>
#include <map>

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

std::deque<SimComponent*> topologicalSort(const std::vector<ComponentGraphic*>& components) {
    std::map<SimComponent*, bool> visited;
    std::deque<SimComponent*> stack;

    for (const auto& cpt : components)
        visited[&cpt->getComponent()] = false;

    for (const auto& c : visited) {
        if (!c.second) {
            topologicalSortUtil(c.first, visited, stack);
        }
    }
    return stack;
}

std::map<int, std::set<SimComponent*>> ASAPSchedule(const std::vector<ComponentGraphic*>& components) {
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

std::map<ComponentGraphic*, QPointF> ASAPPlacement(const std::vector<ComponentGraphic*>& components) {
    std::map<ComponentGraphic*, QPointF> placements;
    const auto asapSchedule = ASAPSchedule(components);

    // 1. create a width of each column
    std::map<int, qreal> columnWidths;
    for (const auto& iter : asapSchedule) {
        qreal maxWidth = 0;
        for (const auto& c : iter.second) {
            qreal width = c->getGraphic<ComponentGraphic>()->boundingRect().width();
            maxWidth = maxWidth < width ? width : maxWidth;
        }
        columnWidths[iter.first] = maxWidth;
    }

    // Position components
    // Start a bit offset from the parent borders
    const QPointF start{50, 25};
    qreal x = start.x();
    qreal y = start.y();
    for (const auto& iter : asapSchedule) {
        for (const auto& c : iter.second) {
            auto* g = c->getGraphic<ComponentGraphic>();
            placements[g] = QPointF(x, y);
            y += g->boundingRect().height() + COMPONENT_COLUMN_MARGIN;
        }
        x += columnWidths[iter.first] + 2 * COMPONENT_COLUMN_MARGIN;
        y = start.y();
    }

    return placements;
}

std::map<ComponentGraphic*, QPointF> topologicalSortPlacement(const std::vector<ComponentGraphic*>& components) {
    std::map<ComponentGraphic*, QPointF> placements;
    std::deque<SimComponent*> sortedComponents = topologicalSort(components);

    // Position components
    QPointF pos = QPointF(25, 25);  // Start a bit offset from the parent borders
    for (const auto& c : sortedComponents) {
        auto* g = c->getGraphic<ComponentGraphic>();
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
        case PlaceAlg::ASAP: {
            return ASAPPlacement(components);
        }
        default:
            Q_ASSERT(false);
    }
}

}  // namespace vsrtl
