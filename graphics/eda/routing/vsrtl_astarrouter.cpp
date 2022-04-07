#include "eda/algorithms/vsrtl_astar.h"
#include "eda/vsrtl_netlist.h"
#include "eda/vsrtl_tilegraph.h"

namespace vsrtl {
namespace eda {

// Define a heuristic cost function for routing tiles
int rrHeuristic(RoutingTile* start, RoutingTile* goal) {
    return 1;  // Todo: fix this
    return (goal->rect().center() - start->rect().center()).manhattanLength();
};

bool validity(RoutingTile* from, RoutingTile* to) {
    return true;  // Todo: fix this
    bool valid;
    const Orientation direction = directionToOrientation(from->adjacentDir(to, valid));
    return valid && to->remainingCap(direction) > 0;
};

std::vector<RoutingTile*> adjacency(RoutingTile* from) {
    /* in the AStar algorithm, we'll allow direct jumps to tiles in the same row and column as @p from. This lets
     * the algorithm implicitly optimize for reducing manhatten distance by selecting long jumps with no direction
     * changes */
    const std::function<void(std::vector<RoutingTile*>&, RoutingTile*, Direction)> getAdjacentRec =
        [&](std::vector<RoutingTile*>& tiles, RoutingTile* rt, Direction edge) {
            if (rt) {
                tiles.push_back(rt);
                getAdjacentRec(tiles, dynamic_cast<RoutingTile*>(rt->neighbour(edge)), edge);
            }
        };

    std::vector<RoutingTile*> rowColTiles;
    for (auto dir : {Direction::South, Direction::North, Direction::West, Direction::East}) {
        getAdjacentRec(rowColTiles, dynamic_cast<RoutingTile*>(from->neighbour(dir)), dir);
    }
    return rowColTiles;
};

void assertValidRoute(const Route& route) {
    assertAdjacentTiles(route.start.routingComponent.get(), *route.path.begin());
    assertAdjacentTiles(*route.path.rbegin(), route.end.routingComponent.get());

    for (size_t i = 0; i < route.path.size(); i++) {
        // Routing components shouldn't be in a list of Routing Tiles
        Q_ASSERT(static_cast<Tile*>(route.start.routingComponent.get()) != static_cast<Tile*>(route.path.at(i)));
        Q_ASSERT(static_cast<Tile*>(route.end.routingComponent.get()) != static_cast<Tile*>(route.path.at(i)));

        if (i > 0) {
            assertAdjacentTiles(route.path.at(i), route.path.at(i - 1));
        }

        for (size_t j = 0; j < route.path.size(); j++) {
            if (i == j) {
                continue;
            }
            Q_ASSERT(route.path.at(i) != route.path.at(j));
        }
    }
}

void AStarRouter(const std::shared_ptr<Netlist>& netlist) {
    // Route via. a* search between start- and stop nodes, using the available routing tiles

    for (auto& net : *netlist) {
        if (net->size() == 0) {
            // Skip empty nets
            continue;
        }

        // Find a route to each start-stop pair in the net
        for (auto& route : *net) {
            route->path = AStar<RoutingTile>(route->start.tile, route->end.tile, adjacency, validity, rrHeuristic);

            // Patch route provided by A-star algorithm to re-add tiles skipped by the modified adjacency function
            for (int i = route->path.size() - 2; i >= 0; i--) {
                bool isAdjacent;
                RoutingTile* curTile = route->path.at(i);
                RoutingTile* preTile = route->path.at(i + 1);
                curTile->adjacentTile(preTile, isAdjacent);
                if (!isAdjacent) {
                    // Patch up intermediate path
                    auto intermediateTiles = curTile->intermediateTiles(preTile);
#ifndef NDEBUG
                    assert(intermediateTiles.size() != 0);
                    assertAdjacentTiles(curTile, *intermediateTiles.begin());
                    assertAdjacentTiles(preTile, *intermediateTiles.rbegin());
#endif
                    route->path.insert(route->path.begin() + i + 1, intermediateTiles.begin(), intermediateTiles.end());
                }
            }
#ifndef NDEBUG
            assertValidRoute(*route);
#endif
        }
    }
}

}  // namespace eda
}  // namespace vsrtl
