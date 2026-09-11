#pragma once

#include "hh/frontend/GameUiTypes.h"
#include <vector>

namespace hh::client {

[[nodiscard]] const std::vector<hh::frontend::BuildCatalogItem>&
liveBuildCatalog();

void attachLiveBuildCatalog(hh::frontend::SimulationSnapshot& snapshot);

} // namespace hh::client
