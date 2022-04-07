#pragma once

#include "vsrtl_tilegraph.h"

namespace vsrtl {
namespace eda {

using Net = std::vector<std::shared_ptr<Route>>;
using Netlist = std::vector<std::shared_ptr<Net>>;

std::shared_ptr<Netlist> createNetlist(const std::shared_ptr<Placement>& placement);

}  // namespace eda
}  // namespace vsrtl
