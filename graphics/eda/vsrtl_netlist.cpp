#include "vsrtl_netlist.h"

#include "vsrtl_gridcomponent.h"

namespace vsrtl {
namespace eda {

std::shared_ptr<Netlist> createNetlist(const std::shared_ptr<TileGraph>& tg) {
    auto netlist = std::make_shared<Netlist>();
    for (const auto& routingComponent : tg->components()) {
        for (const auto& outputPort : routingComponent->gridComponent->getComponent()->getOutputPorts()) {
            // Note: terminal position currently is fixed to right => output, left => input
            auto net = std::make_shared<Net>();
            NetNode source;
            source.port = outputPort;
            source.componentTile = routingComponent;

            // Get source port routing tile
            source.tile = dynamic_cast<RoutingTile*>(
                source.componentTile->neighbour(source.componentTile->gridComponent->getPortPos(outputPort).side));
            Q_ASSERT(source.tile != nullptr);
            for (const auto& sinkPort : outputPort->getOutputPorts()) {
                NetNode sink;
                sink.port = sinkPort;
                GridComponent* sinkGridComponent = sinkPort->getParent()->getGraphic<GridComponent>();
                Q_ASSERT(sinkGridComponent);
                // Lookup routing component for sink component graphic
                auto rc_i = std::find_if(
                    tg->components().begin(), tg->components().end(),
                    [&sinkGridComponent](const auto& rc) { return rc->gridComponent == sinkGridComponent; });

                if (rc_i == tg->components().end()) {
                    /** @todo: connected port is the parent component (i.e., an in- or output port of the parent
                     * component */
                    continue;
                }
                sink.componentTile = *rc_i;

                // Get sink port routing tile
                sink.tile = dynamic_cast<RoutingTile*>(
                    sink.componentTile->neighbour(sink.componentTile->gridComponent->getPortPos(sinkPort).side));
                Q_ASSERT(sink.tile != nullptr);
                net->push_back(std::make_shared<Route>(source, sink));
            }
            netlist->push_back(net);
        }
    }
    return netlist;
}

}  // namespace eda
}  // namespace vsrtl
