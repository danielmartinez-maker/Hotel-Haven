#pragma once

#include "hh/frontend/GameUiTypes.h"
#include <vector>

namespace hh::client {

[[nodiscard]] inline const std::vector<hh::frontend::BuildCatalogItem>&
liveBuildCatalog() {
  // This catalog mirrors only construction commands that the native client can
  // execute today. Materials and labor remain empty/zero because the current
  // immediate-build simulation path does not expose authoritative estimates for
  // either field.
  static const std::vector<hh::frontend::BuildCatalogItem> catalog{
      {"guest_room", "Guest room", "Rooms", 540000, {}, 0},
      {"floor", "Floor", "Structure", 500, {}, 0},
      {"wall", "Wall", "Structure", 500, {}, 0},
      {"door", "Door", "Structure", 500, {}, 0},
      {"guest_entrance", "Guest entrance", "Access", 500, {}, 0},
      {"reception_desk", "Reception desk", "Operations", 500, {}, 0},
      {"supply_closet", "Supply closet", "Operations", 500, {}, 0},
      {"stairs", "Stairs", "Vertical", 500, {}, 0},
      {"remove_tile", "Remove tile", "Demolition", 500, {}, 0},
      {"bathroom", "Bathroom", "Rooms", 500, {}, 0},
      {"staff_room", "Staff room", "Staff", 500, {}, 0},
      {"lobby", "Lobby", "Public", 500, {}, 0},
  };
  return catalog;
}

inline void attachLiveBuildCatalog(hh::frontend::SimulationSnapshot& snapshot) {
  snapshot.buildCatalog = liveBuildCatalog();
}

} // namespace hh::client
