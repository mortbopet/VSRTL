#ifndef VSRTL_PLACEROUTE_H
#define VSRTL_PLACEROUTE_H

#include <QList>
#include <QPointF>
#include <QRect>
#include <map>
#include <vector>

namespace vsrtl {

class ComponentGraphic;
class Component;

struct Placement {
    QRectF chipRect;
    QList<QRectF> components;
};

struct RoutingRegion {
    RoutingRegion(QRectF rect) {
        r = rect;
        h_cap = r.width();
        v_cap = r.height();
    }

    QRectF r;   // Region size and position
    int h_cap;  // Horizontal capacity of routing region
    int v_cap;  // Vertical capacity of routing region

    // Adjacency pointers
    RoutingRegion* top = nullptr;
    RoutingRegion* bottom = nullptr;
    RoutingRegion* left = nullptr;
    RoutingRegion* right = nullptr;

    bool operator==(const RoutingRegion& lhs) const { return r == lhs.r; }
};

QList<RoutingRegion> defineRoutingRegions(const Placement&);

enum class PlaceAlg { TopologicalSort };
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

    PlaceAlg m_placementAlgorithm = PlaceAlg::TopologicalSort;
    RouteAlg m_routingAlgorithm = RouteAlg::Direct;
};
}  // namespace vsrtl

#endif  // VSRTL_PLACEROUTE_H
