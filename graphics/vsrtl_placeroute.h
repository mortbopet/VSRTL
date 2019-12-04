#ifndef VSRTL_PLACEROUTE_H
#define VSRTL_PLACEROUTE_H

#include <QPointF>
#include <map>
#include <vector>

namespace vsrtl {

class ComponentGraphic;

namespace core {
class Component;
}
using namespace core;

enum class PlaceAlg { TopologicalSort, ASAP };
enum class RouteAlg { Direct };
/**
 * @brief The PlaceRoute class
 * Singleton class for containing the various place & route algorithms.
 * Contains state information regarding the current place & route algorithms, as well as the parameters for these.
 * ComponentGraphic's may acces the singleton and query it to perform place & route on the provided subcomponents
 */
class PlaceRoute {
public:
    static const PlaceRoute* get() {
        static PlaceRoute* instance = new PlaceRoute();
        return instance;
    }

    void setPlacementAlgorithm(PlaceAlg alg) { m_placementAlgorithm = alg; }
    void setRoutingAlgorithm(RouteAlg alg) { m_routingAlgorithm = alg; }

    /** @todo: Return a data structure which may be interpreted by the calling ComponentGraphic to place its
     * subcomponents and draw the signal paths. For now, just return a structure suitable for placement*/
    std::map<ComponentGraphic*, QPointF> placeAndRoute(const std::vector<ComponentGraphic*>& components) const;

private:
    PlaceRoute() {}

    PlaceAlg m_placementAlgorithm = PlaceAlg::ASAP;
    RouteAlg m_routingAlgorithm = RouteAlg::Direct;
};
}  // namespace vsrtl

#endif  // VSRTL_PLACEROUTE_H
