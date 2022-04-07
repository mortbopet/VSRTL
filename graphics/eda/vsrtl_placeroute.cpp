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
                    auto edge = route->start.routingComponent->adjacentTile(curTile, valid);
                    Q_ASSERT(valid);
                    const Orientation dir = directionToOrientation(edge);
                    curTile->registerRoute(route.get(), dir);
                }

                if (preTile) {
                    auto edge = preTile->adjacentTile(curTile, valid);
                    Q_ASSERT(valid);
                    const Orientation dir = directionToOrientation(edge);
                    preTile->registerRoute(route.get(), dir);
                    curTile->registerRoute(route.get(), dir);
                }

                if (i == (route->path.size() - 1)) {
                    // last tile
                    auto edge = curTile->adjacentTile(route->end.routingComponent.get(), valid);
                    Q_ASSERT(valid);
                    const Orientation dir = directionToOrientation(edge);
                    curTile->registerRoute(route.get(), dir);
                }

                preTile = curTile;
            }
        }
    }
}

PRResult PlaceRoute::placeAndRoute(const std::vector<GridComponent*>& components) {
    // ======================= PLACE ======================= //
    auto placement = get()->m_placementAlgorithms.at(get()->m_placementAlgorithm)(components);

    // Connectivity graph: Transform current components into a placement format suitable for creating the connectivity
    // graph
    placement->chipRect = placement->componentBoundingRect();
    // Expand chiprect slightly, to reach (0,0) topleft and give equal spacing to the other faces.
    const int hSpacing = placement->chipRect.x();
    const int vSpacing = placement->chipRect.y();
    // Add margins to chip rect to allow routing on right- and bottom borders
    placement->chipRect.adjust(-hSpacing, -vSpacing, hSpacing, vSpacing);
    std::shared_ptr<TileGraph> rGraph = std::make_unique<TileGraph>(placement);

#ifndef NDEBUG
    rGraph->dumpDotFile();
#endif

    // ======================= ROUTE ======================= //
    auto netlist = createNetlist(placement);
    get()->m_routingAlgorithms.at(get()->m_routingAlgorithm)(netlist);

    registerRoutes(netlist);

    rGraph->expandTiles();
    // placement.doTileBasedPlacement();

    // During the routing algorithm, all routes should have registered to their routing tiles. With knowledge of how
    // many routes occupy each routing tile, a route is assigned a lane within the routing tile
    for (const auto& tile : rGraph->verticesOfType<RoutingTile>()) {
        tile->assignRoutes();
    }

    PRResult result;
    result.placement = placement;
    result.tiles = rGraph;
    result.netlist = std::move(netlist);
    return result;
}

}  // namespace eda
}  // namespace vsrtl
