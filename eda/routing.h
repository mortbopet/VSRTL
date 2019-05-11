#ifndef ROUTING_H
#define ROUTING_H

#include <QRect>
#include <map>
#include <vector>

#include "geometry.h"
#include "gridcomponent.h"
#include "gridport.h"

// Various data-structures used during routing

namespace vsrtl {
namespace eda {

class RoutingRegion;

struct NetNode {
    GridComponent* gridComponent = nullptr;
    RoutingRegion* region = nullptr;
    GridPort* port = nullptr;
};

struct Route {
    Route(NetNode s, NetNode e) : start(s), end(e) {}
    NetNode start;
    NetNode end;
    std::vector<RoutingRegion*> path;
};

class RoutingRegion {
public:
    RoutingRegion(QRect rect) : r(rect), h_cap(rect.width()), v_cap(rect.height()) {}

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

class RoutingComponent {
public:
    RoutingComponent(GridComponent* c) : gridComponent(c), rect(gridComponent->adjusted()) {}
    GridComponent* gridComponent;
    const QRect rect;
    RoutingRegion* topRegion = nullptr;
    RoutingRegion* leftRegion = nullptr;
    RoutingRegion* rightRegion = nullptr;
    RoutingRegion* bottomRegion = nullptr;
};

/**
 * @brief The RegionGroup class
 * A region group is the notion of the 4 regions surrounding a horizontal and vertical intersection between region group
 * boundaries.
 * When groups has been associated with all 4 corners, connectRegion() may be called to make the region groups at these
 * 4 corners aware of their adjacent region groups in respect to this point.
 */
class RegionGroup {
public:
    void connectRegions() {
        if (topleft != nullptr) {
            topleft->setRegion(Edge::Bottom, bottomleft);
            topleft->setRegion(Edge::Right, topright);
        }

        if (topright != nullptr) {
            topright->setRegion(Edge::Left, topleft);
            topright->setRegion(Edge::Bottom, bottomright);
        }
        if (bottomleft != nullptr) {
            bottomleft->setRegion(Edge::Top, topleft);
            bottomleft->setRegion(Edge::Right, bottomright);
        }
        if (bottomright != nullptr) {
            bottomright->setRegion(Edge::Left, bottomleft);
            bottomright->setRegion(Edge::Top, topright);
        }
    }
    void setRegion(Corner, RoutingRegion*);

    RoutingRegion* topleft = nullptr;
    RoutingRegion* topright = nullptr;
    RoutingRegion* bottomleft = nullptr;
    RoutingRegion* bottomright = nullptr;
};

struct Placement {
    QRect chipRect;
    std::vector<RoutingComponent> components;
};

#define WRAP_UNIQUEPTR(type) using type##Ptr = std::unique_ptr<type>;
using Net = std::vector<std::unique_ptr<Route>>;
WRAP_UNIQUEPTR(Net)
using Netlist = std::vector<NetPtr>;
WRAP_UNIQUEPTR(Netlist)

using RoutingRegions = std::vector<std::unique_ptr<RoutingRegion>>;

}  // namespace eda
}  // namespace vsrtl

#endif  // ROUTING_H
