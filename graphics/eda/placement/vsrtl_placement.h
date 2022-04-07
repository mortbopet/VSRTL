#pragma once

#include "vsrtl_gridcomponent.h"

namespace vsrtl {
namespace eda {

std::shared_ptr<Placement> MinCutPlacement(const std::vector<GridComponent*>& components);
std::shared_ptr<Placement> topologicalSortPlacement(const std::vector<GridComponent*>& components);
std::shared_ptr<Placement> ASAPPlacement(const std::vector<GridComponent*>& components);

}  // namespace eda
}  // namespace vsrtl
