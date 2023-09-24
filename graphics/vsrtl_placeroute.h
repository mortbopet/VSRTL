#ifndef VSRTL_PLACEROUTE_H
#define VSRTL_PLACEROUTE_H

#include <QPointF>
#include <map>
#include <vector>

namespace vsrtl {

class GridComponent;

enum class PlaceAlg { TopologicalSort, ASAP };
enum class RouteAlg { Direct };
/**
 * @brief The PlaceRoute class
 * Singleton class for containing the various place & route algorithms.
 * Contains state information regarding the current place & route algorithms, as
 * well as the parameters for these. GridComponent's may acces the singleton and
 * query it to perform place & route on the provided subcomponents
 */
class PlaceRoute {
public:
  static const PlaceRoute *get() {
    static PlaceRoute *instance = new PlaceRoute();
    return instance;
  }

  void setPlacementAlgorithm(PlaceAlg alg) { m_placementAlgorithm = alg; }
  void setRoutingAlgorithm(RouteAlg alg) { m_routingAlgorithm = alg; }

  /** @todo: Return a data structure which may be interpreted by the calling
   * GridComponent to place its subcomponents and draw the signal paths. For
   * now, just return a structure suitable for placement*/
  std::map<GridComponent *, QPoint>
  placeAndRoute(const std::vector<GridComponent *> &components) const;

private:
  PlaceRoute() {}

  PlaceAlg m_placementAlgorithm = PlaceAlg::ASAP;
  RouteAlg m_routingAlgorithm = RouteAlg::Direct;
};
} // namespace vsrtl

#endif // VSRTL_PLACEROUTE_H
