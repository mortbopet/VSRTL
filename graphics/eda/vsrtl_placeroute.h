#ifndef VSRTL_PLACEROUTE_H
#define VSRTL_PLACEROUTE_H

#include <QPointF>

#include <map>
#include <vector>

#include "vsrtl_routing.h"

namespace vsrtl {
class GridComponent;
namespace eda {

enum class PlaceAlg { ASAP, Topological1D, MinCut };
enum class RouteAlg { Direct };

using PlacementFunct = std::function<Placement(const std::vector<GridComponent*>&)>;

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

    std::map<PlaceAlg, PlacementFunct> m_placementAlgorithms;

    PlaceAlg m_placementAlgorithm = PlaceAlg::Topological1D;
    RouteAlg m_routingAlgorithm = RouteAlg::Direct;
};

}  // namespace eda
}  // namespace vsrtl

#endif  // VSRTL_PLACEROUTE_H
