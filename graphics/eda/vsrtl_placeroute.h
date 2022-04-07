#ifndef VSRTL_PLACEROUTE_H
#define VSRTL_PLACEROUTE_H

#include <QPointF>

#include <map>
#include <memory>
#include <vector>

#include "vsrtl_netlist.h"
#include "vsrtl_tilegraph.h"

namespace vsrtl {
class GridComponent;
namespace eda {

enum class PlaceAlg { ASAP, Topological1D, MinCut };
enum class RouteAlg { Direct, AStar };

using PlacementFunct = std::function<std::shared_ptr<Placement>(const std::vector<GridComponent*>&)>;
using RouteFunct = std::function<void(std::shared_ptr<Netlist>&)>;

/// The PRResult class is used to store the result of the placement and routing
/// algorithms.
struct PRResult {
    std::shared_ptr<Placement> placement;
    std::shared_ptr<TileGraph> tiles;
    std::shared_ptr<Netlist> netlist;
};

/**
 * @brief The PlaceRoute class
 * Singleton class for containing the various place & route algorithms.
 * Contains state information regarding the current place & route algorithms, as well as the parameters for these.
 * GridComponent's may acces the singleton and query it to perform place & route on the provided subcomponents
 */
class PlaceRoute {
public:
    static const PlaceRoute* get() {
        static PlaceRoute* instance = new PlaceRoute();
        return instance;
    }

    void setPlacementAlgorithm(PlaceAlg alg) { m_placementAlgorithm = alg; }
    void setRoutingAlgorithm(RouteAlg alg) { m_routingAlgorithm = alg; }

    static PRResult placeAndRoute(const std::vector<GridComponent*>& components);

private:
    PlaceRoute();
    PRResult routeAndExpand(std::shared_ptr<Placement> placement) const;

    std::map<PlaceAlg, PlacementFunct> m_placementAlgorithms;
    std::map<RouteAlg, RouteFunct> m_routingAlgorithms;

    PlaceAlg m_placementAlgorithm = PlaceAlg::Topological1D;
    RouteAlg m_routingAlgorithm = RouteAlg::AStar;
};

}  // namespace eda
}  // namespace vsrtl

#endif  // VSRTL_PLACEROUTE_H
