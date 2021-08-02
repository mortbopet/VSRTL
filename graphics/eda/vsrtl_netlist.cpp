#include "vsrtl_netlist.h"

#include "vsrtl_gridcomponent.h"

namespace vsrtl {
namespace eda {

NetlistPtr createNetlist(Placement& placement) {
    auto netlist = std::make_unique<Netlist>();
    for (const auto& routingComponent : placement.components) {
        for (const auto& outputPort : routingComponent->gridComponent->getComponent()->getOutputPorts()) {
            // Note: terminal position currently is fixed to right => output, left => input
            auto net = std::make_unique<Net>();
            NetNode source;
            source.port = outputPort;
            source.routingComponent = routingComponent;

            // Get source port routing tile
            source.tile = dynamic_cast<RoutingTile*>(source.routingComponent->neighbour(
                source.routingComponent->gridComponent->getPortPos(outputPort).side));
            Q_ASSERT(source.tile != nullptr);
            for (const auto& sinkPort : outputPort->getOutputPorts()) {
                NetNode sink;
                sink.port = sinkPort;
                GridComponent* sinkGridComponent = sinkPort->getParent()->getGraphic<GridComponent>();
                Q_ASSERT(sinkGridComponent);
                // Lookup routing component for sink component graphic
                auto rc_i = std::find_if(
                    placement.components.begin(), placement.components.end(),
                    [&sinkGridComponent](const auto& rc) { return rc->gridComponent == sinkGridComponent; });

                if (rc_i == placement.components.end()) {
                    /** @todo: connected port is the parent component (i.e., an in- or output port of the parent
                     * component */
                    continue;
                }
                sink.routingComponent = *rc_i;

                // Get sink port routing tile
                sink.tile = dynamic_cast<RoutingTile*>(
                    sink.routingComponent->neighbour(sink.routingComponent->gridComponent->getPortPos(sinkPort).side));
                Q_ASSERT(sink.tile != nullptr);
                net->push_back(std::make_unique<Route>(source, sink));
            }
            netlist->push_back(std::move(net));
        }
    }
    return netlist;
}

}
}