#pragma once

#include "placement/vsrtl_placement.h"
#include "vsrtl_tilegraph.h"

namespace vsrtl {
namespace eda {

using Net = std::vector<std::shared_ptr<Route>>;
using Netlist = std::vector<std::shared_ptr<Net>>;

std::shared_ptr<Netlist> createNetlist(const std::shared_ptr<TileGraph>& tg);

}  // namespace eda
}  // namespace vsrtl
