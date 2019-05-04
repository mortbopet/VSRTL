#ifndef VSRTL_PLACEROUTE_H
#define VSRTL_PLACEROUTE_H

#include <QList>
#include <QPointF>
#include <QRect>
#include <map>
#include <memory>
#include <vector>

namespace vsrtl {
class ComponentGraphic;
class PortGraphic;
class Component;

namespace pr {

enum class Edge { Top, Bottom, Left, Right };
enum class Direction { Horizontal, Vertical };
enum class Corner { TopLeft, TopRight, BottomRight, BottomLeft };

struct Route;
class RoutingRegion;

/**
 * @brief The RegionGroup class
 * A region group is the notion of the 4 regions surrounding a horizontal and vertical intersection between region group
 * boundaries.
 * When groups has been associated with all 4 corners, connectRegion() may be called to make the region groups at these
 * 4 corners aware of their adjacent region groups in respect to this point.
 */
class RegionGroup {
public:
    void connectRegions();
    void setRegion(Corner, RoutingRegion*);

    RoutingRegion* topleft = nullptr;
    RoutingRegion* topright = nullptr;
    RoutingRegion* bottomleft = nullptr;
    RoutingRegion* bottomright = nullptr;
};

struct RouteAssignment {
    Direction dir;
    float index;
};

class RoutingRegion {
public:
    RoutingRegion(QRect rect) : h_cap(rect.width()), v_cap(rect.height()), r(rect) {}

    const QRect& rect() { return r; }
    const std::vector<RoutingRegion*> adjacentRegions();
    void setRegion(Edge, RoutingRegion*);
    void assignRoutes();
    void registerRoute(Route*, Direction);

    static inline bool cmpRoutingRegPtr(RoutingRegion* a, RoutingRegion* b) {
        if ((a == nullptr && b != nullptr) || (b == nullptr && a != nullptr))
            return false;
        if (a == nullptr && b == nullptr)
            return true;
        return a->r == b->r;
    }

    bool operator==(const RoutingRegion& lhs) const {
        if (!cmpRoutingRegPtr(top, lhs.top))
            return false;
        if (!cmpRoutingRegPtr(bottom, lhs.bottom))
            return false;
        if (!cmpRoutingRegPtr(left, lhs.left))
            return false;
        if (!cmpRoutingRegPtr(right, lhs.right))
            return false;

        return r == lhs.r;
    }

private:
    std::map<Route*, RouteAssignment> assignedRoutes;
    std::vector<Route*> verticalRoutes, horizontalRoutes;

    // Adjacent region groups
    RoutingRegion* top = nullptr;
    RoutingRegion* bottom = nullptr;
    RoutingRegion* left = nullptr;
    RoutingRegion* right = nullptr;

    QRect r;    // Region size and position
    int h_cap;  // Horizontal capacity of routing region
    int v_cap;  // Vertical capacity of routing region
    int h_used = 0;
    int v_used = 0;
};

class RoutingComponent : public QRect {
public:
    using QRect::QRect;
    using QRect::operator=;

    ComponentGraphic* componentGraphic;
    RoutingRegion* topRegion = nullptr;
    RoutingRegion* leftRegion = nullptr;
    RoutingRegion* rightRegion = nullptr;
    RoutingRegion* bottomRegion = nullptr;
};

struct Placement {
    QRect chipRect;
    QList<RoutingComponent> components;
};

struct NetNode {
    ComponentGraphic* componentGraphic = nullptr;
    PortGraphic* port = nullptr;
    RoutingRegion* region = nullptr;
    Edge edgePos;
    unsigned int edgeIndex;
};

struct Route {
    Route(NetNode s, NetNode e) : start(s), end(e) {}
    NetNode start;
    NetNode end;
    std::vector<RoutingRegion*> path;
};

#define WRAP_UNIQUEPTR(type) using type##Ptr = std::unique_ptr<type>;

using Net = std::vector<std::unique_ptr<Route>>;
WRAP_UNIQUEPTR(Net)
using Netlist = std::vector<NetPtr>;
WRAP_UNIQUEPTR(Netlist)
using RoutingRegions = std::vector<std::unique_ptr<RoutingRegion>>;

RoutingRegions createConnectivityGraph(Placement&);

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
    static PlaceRoute& get() {
        static PlaceRoute* instance = new PlaceRoute();
        return *instance;
    }

    void setPlacementAlgorithm(PlaceAlg alg) { m_placementAlgorithm = alg; }
    void setRoutingAlgorithm(RouteAlg alg) { m_routingAlgorithm = alg; }

    /** @todo: Return a data structure which may be interpreted by the calling ComponentGraphic to place its
     * subcomponents and draw the signal paths. For now, just return a structure suitable for placement*/
    void placeAndRoute(const std::vector<ComponentGraphic*>& components, RoutingRegions& regions);

private:
    PlaceRoute() {}

    PlaceAlg m_placementAlgorithm = PlaceAlg::TopologicalSort;
    RouteAlg m_routingAlgorithm = RouteAlg::Direct;
};
}  // namespace pr
}  // namespace vsrtl

#endif  // VSRTL_PLACEROUTE_H
