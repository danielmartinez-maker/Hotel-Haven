#pragma once

#include "BuildTool.h"
#include "hh/frontend/GameUiTypes.h"
#include <optional>
#include <string_view>
#include <vector>

namespace hh::client {

struct LiveBuildCatalogEntry {
  Tool tool{Tool::Inspect};
  hh::frontend::BuildCatalogItem item;
};

[[nodiscard]] inline const std::vector<LiveBuildCatalogEntry>&
liveBuildCatalogEntries() {
  // IDs intentionally match the canonical item ids emitted by the native
  // placement-preview path. Materials and labor remain empty/zero because the
  // current immediate-build simulation path exposes no authoritative estimates
  // for either field.
  static const std::vector<LiveBuildCatalogEntry> entries{
      {Tool::Bedroom, {"Guest room", "Guest room", "Rooms", 540000, {}, 0}},
      {Tool::Floor, {"Floor", "Floor", "Structure", 500, {}, 0}},
      {Tool::Wall, {"Wall", "Wall", "Structure", 500, {}, 0}},
      {Tool::Door, {"Door", "Door", "Structure", 500, {}, 0}},
      {Tool::Entrance, {"Guest entrance", "Guest entrance", "Access", 500, {}, 0}},
      {Tool::Desk, {"Reception desk", "Reception desk", "Operations", 500, {}, 0}},
      {Tool::Closet, {"Supply closet", "Supply closet", "Operations", 500, {}, 0}},
      {Tool::Stairs, {"Stairs", "Stairs", "Vertical", 500, {}, 0}},
      {Tool::Erase, {"Remove tile", "Remove tile", "Demolition", 500, {}, 0}},
      {Tool::Bathroom, {"Bathroom", "Bathroom", "Rooms", 500, {}, 0}},
      {Tool::StaffRoom, {"Staff room", "Staff room", "Staff", 500, {}, 0}},
      {Tool::Lobby, {"Lobby", "Lobby", "Public", 500, {}, 0}},
  };
  return entries;
}

[[nodiscard]] inline const std::vector<hh::frontend::BuildCatalogItem>&
liveBuildCatalog() {
  static const std::vector<hh::frontend::BuildCatalogItem> catalog = [] {
    std::vector<hh::frontend::BuildCatalogItem> items;
    items.reserve(liveBuildCatalogEntries().size());
    for (const auto& entry : liveBuildCatalogEntries())
      items.push_back(entry.item);
    return items;
  }();
  return catalog;
}

[[nodiscard]] inline std::optional<Tool> liveBuildTool(std::string_view itemId) {
  for (const auto& entry : liveBuildCatalogEntries())
    if (entry.item.id == itemId)
      return entry.tool;
  return std::nullopt;
}

inline void attachLiveBuildCatalog(hh::frontend::SimulationSnapshot& snapshot) {
  snapshot.buildCatalog = liveBuildCatalog();
}

} // namespace hh::client
