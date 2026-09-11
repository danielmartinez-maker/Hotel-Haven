#pragma once

#include "hh/frontend/GameUiTypes.h"
#include <vector>

namespace hh::client {

[[nodiscard]] inline const std::vector<hh::frontend::BuildCatalogItem>&
liveBuildCatalog() {
  // IDs intentionally match the canonical tool names emitted by the native
  // placement-preview path. Materials and labor remain empty/zero because the
  // current immediate-build simulation path exposes no authoritative estimates
  // for either field.
  static const std::vector<hh::frontend::BuildCatalogItem> catalog{
      {"Guest room", "Guest room", "Rooms", 540000, {}, 0},
      {"Floor", "Floor", "Structure", 500, {}, 0},
      {"Wall", "Wall", "Structure", 500, {}, 0},
      {"Door", "Door", "Structure", 500, {}, 0},
      {"Guest entrance", "Guest entrance", "Access", 500, {}, 0},
      {"Reception desk", "Reception desk", "Operations", 500, {}, 0},
      {"Supply closet", "Supply closet", "Operations", 500, {}, 0},
      {"Stairs", "Stairs", "Vertical", 500, {}, 0},
      {"Remove tile", "Remove tile", "Demolition", 500, {}, 0},
      {"Bathroom", "Bathroom", "Rooms", 500, {}, 0},
      {"Staff room", "Staff room", "Staff", 500, {}, 0},
      {"Lobby", "Lobby", "Public", 500, {}, 0},
  };
  return catalog;
}

inline void attachLiveBuildCatalog(hh::frontend::SimulationSnapshot& snapshot) {
  snapshot.buildCatalog = liveBuildCatalog();
}

} // namespace hh::client
