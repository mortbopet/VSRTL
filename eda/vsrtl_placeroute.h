#ifndef VSRTL_PLACEROUTE_H
#define VSRTL_PLACEROUTE_H

#include <QList>
#include <QPointF>
#include <QRect>
#include <map>
#include <memory>
#include <set>
#include <vector>

#include "geometry.h"
#include "gridcomponent.h"
#include "routing.h"

namespace vsrtl {
class ComponentGraphic;
class PortGraphic;
class Component;

namespace eda {

RoutingRegions createConnectivityGraph(Placement&);

enum class PlaceAlg { Topological1D, MinCut };
enum class RouteAlg { Direct };
/**
 * @brief The PlaceRoute class
 * Singleton class for containing the various place & route algorithms.
 * Contains state information regarding the current place & route algorithms, as well as the parameters for these.
 * ComponentGraphic's may acces the singleton and query it to perform place & route on the provided subcomponents
 */
class PlaceRoute {
public:
    static PlaceRoute& get() {
        static PlaceRoute* instance = new PlaceRoute();
        return *instance;
    }

    void setPlacementAlgorithm(PlaceAlg alg) { m_placementAlgorithm = alg; }
    void setRoutingAlgorithm(RouteAlg alg) { m_routingAlgorithm = alg; }

    /** @todo: Return a data structure which may be interpreted by the calling ComponentGraphic to place its
     * subcomponents and draw the signal paths. For now, just return a structure suitable for placement*/
    void placeAndRoute(const std::vector<GridComponent*>& components, RoutingRegions& regions);

private:
    PlaceRoute() {}

    PlaceAlg m_placementAlgorithm = PlaceAlg::Topological1D;
    RouteAlg m_routingAlgorithm = RouteAlg::Direct;
};
}  // namespace eda
}  // namespace vsrtl

#endif  // VSRTL_PLACEROUTE_H
