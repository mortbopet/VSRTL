#pragma once

#include "vsrtl_tilegraph.h"

namespace vsrtl {
namespace eda {

using Net = std::vector<std::unique_ptr<Route>>;
WRAP_UNIQUEPTR(Net)
using Netlist = std::vector<NetPtr>;
WRAP_UNIQUEPTR(Netlist)

NetlistPtr createNetlist(Placement& placement);

}  // namespace eda
}  // namespace vsrtl
