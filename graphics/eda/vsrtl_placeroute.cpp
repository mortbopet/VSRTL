#include "vsrtl_placeroute.h"

#include <deque>
#include <map>

#include "vsrtl_component.h"
#include "vsrtl_geometry.h"
#include "vsrtl_graphics_defines.h"
#include "vsrtl_graphics_util.h"
#include "vsrtl_gridcomponent.h"
#include "vsrtl_portgraphic.h"
#include "vsrtl_wiregraphic.h"

#include "vsrtl_astar.h"
#include "vsrtl_mincutplacement.cpp"

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

Placement ASAPPlacement(const std::vector<GridComponent*>& components) {
    Placement placements;
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
            placements.components.push_back(std::make_shared<RoutingComponent>(routable));
            y += g->getCurrentComponentRect().height() + COMPONENT_COLUMN_MARGIN;
        }
        x += columnWidths[iter.first] + 2 * COMPONENT_COLUMN_MARGIN;
        y = start.y();
    }

    return placements;
}

Placement topologicalSortPlacement(const std::vector<GridComponent*>& components) {
    Placement placements;
    std::deque<SimComponent*> sortedComponents = topologicalSort(components);

    // Position components
    QPoint pos = QPoint(SUBCOMPONENT_INDENT, SUBCOMPONENT_INDENT);  // Start a bit offset from the parent borders
    for (const auto& c : sortedComponents) {
        auto* g = c->getGraphic<GridComponent>();
        auto routable = RoutingComponent(g);
        routable.pos = pos;
        placements.components.push_back(std::make_shared<RoutingComponent>(routable));
        pos.rx() += g->getCurrentComponentRect().width() + COMPONENT_COLUMN_MARGIN;
    }

    return placements;
}

PlaceRoute::PlaceRoute() {
    m_placementAlgorithms[PlaceAlg::ASAP] = &ASAPPlacement;
    m_placementAlgorithms[PlaceAlg::Topological1D] = &topologicalSortPlacement;
    m_placementAlgorithms[PlaceAlg::MinCut] = &MinCutPlacement;

    m_placementAlgorithm = PlaceAlg::ASAP;
}

void PlaceRoute::assignRoutes(NetlistPtr& netlist) const {
    for (const auto& net : *netlist.get()) {
        for (const auto& route : *net.get()) {
            Q_ASSERT(route->start.port != nullptr && route->end.port != nullptr);
            auto startPort = route->start.port->getGraphic<PortGraphic>();
            auto endPort = route->end.port->getGraphic<PortGraphic>();

            Q_ASSERT(startPort);
            auto* wire = startPort->getOutputWire();
            wire->clearWirePoints();
            Q_ASSERT(wire->getWires().size() == 0);
            WireSegment* seg = wire->createSegment(startPort->getPortPoint(SimPort::PortType::out),
                                                   endPort->getPortPoint(SimPort::PortType::in));

            for (const auto& region : route->path) {
                auto path = region->getPath(route.get());
                auto [newPoint1, newSeg1] = wire->createWirePointOnSeg(path.from(), seg);
                auto [newPoint2, newSeg2] = wire->createWirePointOnSeg(path.to(), newSeg1);
                Q_UNUSED(newPoint1);
                Q_UNUSED(newPoint2);
                seg = newSeg2;
            }
        }
    }
}

void PlaceRoute::placeAndRoute(const std::vector<GridComponent*>& components) {
    // Placement
    Placement placement = get()->m_placementAlgorithms.at(get()->m_placementAlgorithm)(components);

    // Connectivity graph: Transform current components into a placement format suitable for creating the connectivity
    // graph
    placement.chipRect = placement.componentBoundingRect();
    // Add margins to chip rect to allow routing on right- and bottom borders
    placement.chipRect.adjust(-placement.chipRect.width() / 4, -placement.chipRect.height() / 4,
                              placement.chipRect.width() / 4, placement.chipRect.height() / 4);
    auto cGraph = createConnectivityGraph(placement);

    // Indexable region map
    const auto regionMap = RegionMap(cGraph);

    // ======================= ROUTING ======================= //
    // Define a heuristic cost function for routing regions
    const auto rrHeuristic = [](RoutingRegion* start, RoutingRegion* goal) {
        return (goal->rect().center() - start->rect().center()).manhattanLength();
    };
    auto netlist = createNetlist(placement, regionMap);

    // Route via. a* search between start- and stop nodes, using the available routing regions
    for (auto& net : *netlist) {
        if (net->size() == 0) {
            // Skip empty nets
            continue;
        }

        // Find a route to each start-stop pair in the net
        for (auto& route : *net) {
            route->path = AStar<RoutingRegion>(route->start.region, route->end.region, &RoutingRegion::adjacentRegions,
                                               rrHeuristic);
            // For each region that the route passes through, register the route and its direction within it

            RoutingRegion* preRegion = nullptr;
            bool valid;
            for (unsigned i = 0; i < route->path.size(); i++) {
                const bool lastRegion = i == (route->path.size() - 1);
                RoutingRegion* curRegion = route->path.at(i);

                if (preRegion) {
                    auto edge = preRegion->adjacentRegion(curRegion, valid);
                    Q_ASSERT(valid);
                    preRegion->registerRoute(route.get(), edgeToDirection(edge));
                    if (lastRegion) {
                        curRegion->registerRoute(route.get(), edgeToDirection(edge));
                    }
                } else if (lastRegion) {
                    // Directly abutting regions
                    curRegion->registerRoute(route.get(), Direction::Horizontal);
                }
                preRegion = curRegion;
            }
        }
    }

    // Move components to their final positions
    for (const auto& p : placement.components) {
        p->gridComponent->move(p->pos);
    }

    // During findRoute, all routes have registered to their routing regions. With knowledge of how many routes occupy
    // each routing region, a route is assigned a lane within the routing region
    for (const auto& region : cGraph) {
        region->assignRoutes();
    }

    // Apply assigned routes
    get()->assignRoutes(netlist);
}

}  // namespace vsrtl
