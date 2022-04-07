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
                    auto edge = curTile->isAdjacentTile(route->end.componentTile.get(), valid);
                    Q_ASSERT(valid);
                    const Orientation dir = directionToOrientation(edge);
                    curTile->registerRoute(route.get(), dir);
                }

                preTile = curTile;
            }
        }
    }
}

PRResult PlaceRoute::routeAndExpand(std::shared_ptr<Placement> placement) const {
    // Connectivity graph: Transform current components into a placement format suitable for creating the connectivity
    // graph
    // Add margins to chip rect to allow routing on right- and bottom borders
    std::shared_ptr<TileGraph> rGraph = std::make_shared<TileGraph>(placement);

#ifndef NDEBUG
    rGraph->dumpDotFile();
#endif

    // ======================= ROUTE ======================= //
    auto netlist = createNetlist(placement);
    PlaceRoute::get()->m_routingAlgorithms.at(PlaceRoute::get()->m_routingAlgorithm)(netlist);

    registerRoutes(netlist);

    bool expanded = rGraph->expandTiles();
    if (expanded) {
        // Tiles in the graph were expanded to accomodate the assigned routing. In this case, we recurse with a new
        // placement based on the expanded tiles. (The hope is that eventually, we've expanded enough tiles => recreated
        // a placement for our components => a routing can be done without needing to expand any tiles).
        // Set a new chiprect based on the current bounding rect of the TileGraph
        placement->setChipRect(rGraph->boundingRect());
        rGraph.reset();
        netlist.reset();
        return routeAndExpand(placement);
    }
    // placement.doTileBasedPlacement();

    // During the routing algorithm, all routes should have registered to their routing tiles. With knowledge of how
    // many routes occupy each routing tile, a route is assigned a lane within the routing tile
    for (const auto& tile : rGraph->vertices<RoutingTile>()) {
        tile->assignRoutes();
    }

    PRResult result;
    result.placement = placement;
    result.tiles = rGraph;
    result.netlist = std::move(netlist);
    return result;
}

PRResult PlaceRoute::placeAndRoute(const std::vector<GridComponent*>& components) {
    // Generate an initial placement. This will be used as the starting point for the routing algorithm + tile
    // expansion.
    auto placement = get()->m_placementAlgorithms.at(get()->m_placementAlgorithm)(components);
    QRect chipRect = placement->componentBoundingRect();
    // Expand chiprect slightly, to reach (0,0) topleft and give equal spacing to the other faces.
    const int hSpacing = chipRect.x();
    const int vSpacing = chipRect.y();
    chipRect.adjust(-hSpacing, -vSpacing, hSpacing, vSpacing);
    placement->setChipRect(chipRect);

    return get()->routeAndExpand(placement);
}

}  // namespace eda
}  // namespace vsrtl
