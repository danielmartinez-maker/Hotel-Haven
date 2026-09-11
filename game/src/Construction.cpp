#include "hh/game/Construction.h"
#include <algorithm>
#include <array>

namespace hh::game::detail {
namespace {
constexpr std::array<ConstructionDefinition, 10> definitions{{
    {"chair", 1, 1, ConstructionSupport::Floor, false, false, 7500,
     {1, 0, 0, 0, 1}},
    {"desk", 2, 1, ConstructionSupport::Floor, true, true, 15000,
     {2, 0, 0, 0, 1}},
    {"guest_bed", 2, 1, ConstructionSupport::Floor, true, true, 25000,
     {2, 1, 0, 0, 2}},
    {"wall_sconce", 1, 1, ConstructionSupport::Wall, false, false, 8000,
     {0, 0, 1, 0, 1}},
    {"power_source", 1, 1, ConstructionSupport::Floor, true, true, 30000,
     {0, 0, 3, 0, 2}},
    {"water_source", 1, 1, ConstructionSupport::Floor, true, true, 30000,
     {0, 0, 0, 3, 2}},
    {"fire_alarm", 1, 1, ConstructionSupport::Wall, false, false, 12000,
     {0, 0, 1, 0, 1}},
    {"security_camera", 1, 1, ConstructionSupport::Wall, false, false, 16000,
     {0, 0, 1, 0, 2}},
    {"passenger_elevator", 1, 1, ConstructionSupport::Floor, true, true,
     100000, {1, 1, 3, 1, 4}},
    {"service_elevator", 1, 1, ConstructionSupport::Floor, true, true,
     120000, {2, 1, 3, 1, 5}},
}};
} // namespace

const ConstructionDefinition *
constructionDefinition(std::string_view typeId) noexcept {
  const auto it = std::find_if(definitions.begin(), definitions.end(),
                               [&](const ConstructionDefinition &definition) {
                                 return definition.typeId == typeId;
                               });
  return it == definitions.end() ? nullptr : &*it;
}

int normalizedQuarterTurns(int turns) noexcept {
  turns %= 4;
  if (turns < 0)
    turns += 4;
  return turns;
}

std::vector<Position>
constructionFootprint(const ConstructionPlacement &placement,
                      const ConstructionDefinition &definition) {
  const int rotation = normalizedQuarterTurns(placement.rotationQuarterTurns);
  const int width = rotation % 2 == 0 ? definition.width : definition.height;
  const int height = rotation % 2 == 0 ? definition.height : definition.width;
  std::vector<Position> footprint;
  footprint.reserve(static_cast<std::size_t>(width * height));
  for (int y = 0; y < height; ++y)
    for (int x = 0; x < width; ++x)
      footprint.push_back(
          {placement.origin.floor, placement.origin.x + x,
           placement.origin.y + y});
  return footprint;
}

} // namespace hh::game::detail
