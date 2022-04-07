#pragma once

#include "eda/vsrtl_netlist.h"
#include "eda/vsrtl_tilegraph.h"

namespace vsrtl {
namespace eda {

void AStarRouter(const std::shared_ptr<Netlist>& netlist);

}
}  // namespace vsrtl
