#include "vsrtl_placeroute.h"

#include <map>

#include "vsrtl_component.h"
#include "vsrtl_geometry.h"
#include "vsrtl_graphics_defines.h"
#include "vsrtl_graphics_util.h"
#include "vsrtl_gridcomponent.h"
#include "vsrtl_portgraphic.h"
#include "vsrtl_wiregraphic.h"

#include "placement/vsrtl_placement.h"
#include "routing/vsrtl_routing.h"

#include <QDebug>

namespace vsrtl {
namespace eda {

PlaceRoute::PlaceRoute() {
    m_placementAlgorithms[PlaceAlg::ASAP] = &ASAPPlacement;
    m_placementAlgorithms[PlaceAlg::Topological1D] = &topologicalSortPlacement;
    m_placementAlgorithms[PlaceAlg::MinCut] = &MinCutPlacement;

    m_routingAlgorithms[RouteAlg::AStar] = &AStarRouter;

    m_placementAlgorithm = PlaceAlg::ASAP;
    m_routingAlgorithm = RouteAlg::AStar;
}

void registerRoutes(const std::shared_ptr<Netlist>& netlist) {
    // For each tile that the route passes through, register the route and its direction within it
    for (auto& net : *netlist) {
        if (net->size() == 0) {
            // Skip empty nets
            continue;
        }

        for (auto& route : *net) {
            RoutingTile* preTile = nullptr;
            bool valid;
            for (unsigned i = 0; i < route->path.size(); i++) {
                RoutingTile* curTile = route->path.at(i);

                if (i == 0) {
                    // first tile
                    auto edge = route->start.componentTile->isAdjacentTile(curTile, valid);
                    Q_ASSERT(valid);
                    const Orientation dir = directionToOrientation(edge);
                    curTile->registerRoute(route.get(), dir);
                }

                if (preTile) {
                    auto edge = preTile->isAdjacentTile(curTile, valid);
                    Q_ASSERT(valid);
                    const Orientation dir = directionToOrientation(edge);
                    preTile->registerRoute(route.get(), dir);
                    curTile->registerRoute(route.get(), dir);
                }

                if (i == (route->path.size() - 1)) {
                    // last tile
                    auto edge = curTile->isAdjacentTile(route->end.componentTile, valid);
                    Q_ASSERT(valid);
                    const Orientation dir = directionToOrientation(edge);
                    curTile->registerRoute(route.get(), dir);
                }

                preTile = curTile;
            }
        }
    }
}

std::shared_ptr<PRResult> PlaceRoute::routeAndExpand(std::shared_ptr<Placement> placement) const {
    qDebug() << "Route and expanding";
    // Connectivity graph: Transform current components into a placement format suitable for creating the connectivity
    // graph
    // Add margins to chip rect to allow routing on right- and bottom borders
    std::shared_ptr<TileGraph> rGraph = placement->toTileGraph();

#ifndef NDEBUG
    rGraph->dumpDotFile();
#endif

    // ======================= ROUTE ======================= //
    auto netlist = createNetlist(rGraph);
    PlaceRoute::get()->m_routingAlgorithms.at(PlaceRoute::get()->m_routingAlgorithm)(netlist);

    registerRoutes(netlist);

    (void)rGraph->expandTiles();
    // placement.doTileBasedPlacement();

    // During the routing algorithm, all routes should have registered to their routing tiles. With knowledge of how
    // many routes occupy each routing tile, a route is assigned a lane within the routing tile
    for (const auto& tile : rGraph->vertices<RoutingTile>()) {
        tile->assignRoutes();
    }

    auto result = std::make_shared<PRResult>();
    result->placement = placement;
    result->tiles = rGraph;
    result->netlist = std::move(netlist);
    return result;
}

std::shared_ptr<PRResult> PlaceRoute::placeAndRoute(const std::vector<GridComponent*>& components) {
    // Generate an initial placement. This will be used as the starting point for the routing algorithm + tile
    // expansion.
    auto placement = get()->m_placementAlgorithms.at(get()->m_placementAlgorithm)(components);

    return get()->routeAndExpand(placement);
}

}  // namespace eda
}  // namespace vsrtl
