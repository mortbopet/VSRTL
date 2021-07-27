#include "vsrtl_astar.h"
#include "vsrtl_routing.h"

namespace vsrtl {
namespace eda {

// Define a heuristic cost function for routing tiles
int rrHeuristic(const RoutingTile* start, const RoutingTile* goal) {
    return 1;  // Todo: fix this
    return (goal->rect().center() - start->rect().center()).manhattanLength();
};

bool validity(const RoutingTile* from, const RoutingTile* to) {
    return true;  // Todo: fix this
    const Direction direction = directionBetweenRRs(from, to);
    return to->remainingCap(direction) > 0;
};

std::vector<RoutingTile*> adjacency(RoutingTile* from) {
    /* in the AStar algorithm, we'll allow direct jumps to tiles in the same row and column as @p from. This lets
     * the algorithm implicitly optimize for reducing manhatten distance by selecting long jumps with no direction
     * changes */
    const std::function<void(std::vector<RoutingTile*>&, RoutingTile*, Edge)> getAdjacentRec =
        [&](std::vector<RoutingTile*>& tiles, RoutingTile* rt, Edge edge) {
            if (rt) {
                tiles.push_back(rt);
                getAdjacentRec(tiles, dynamic_cast<RoutingTile*>(rt->getAdjacentTile(edge)), edge);
            }
        };

    std::vector<RoutingTile*> rowColTiles;
    for (auto dir : {Edge::Bottom, Edge::Top, Edge::Left, Edge::Right}) {
        getAdjacentRec(rowColTiles, dynamic_cast<RoutingTile*>(from->getAdjacentTile(dir)), dir);
    }
    return rowColTiles;
};

void AStarRouter(NetlistPtr& netlist) {
    // Route via. a* search between start- and stop nodes, using the available routing tiles

    for (auto& net : *netlist) {
        if (net->size() == 0) {
            // Skip empty nets
            continue;
        }

        // Find a route to each start-stop pair in the net
        for (auto& route : *net) {
            route->path = AStar<RoutingTile>(route->start.tile, route->end.tile, adjacency, validity, rrHeuristic);
        }
    }
}

}  // namespace eda
}  // namespace vsrtl
