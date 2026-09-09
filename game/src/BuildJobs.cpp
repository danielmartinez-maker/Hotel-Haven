#include "hh/game/BuildJobs.h"
#include <algorithm>
#include <array>
#include <limits>

namespace hh::game::detail {
ConstructionMaterials
constructionMaterialsFor(const ConstructionCommand &command) {
  ConstructionMaterials result;
  auto add = [&](const std::array<int, 5> &units) {
    result.lumber += units[0];
    result.drywall += units[1];
    result.electrical += units[2];
    result.plumbing += units[3];
    result.hardware += units[4];
  };
  for (const auto &placement : command.placements)
    if (const auto *definition = constructionDefinition(placement.typeId))
      add(definition->materialUnits);
  return result;
}

bool hasMaterials(const ConstructionMaterials &available,
                  const ConstructionMaterials &required) noexcept {
  return available.lumber >= required.lumber &&
         available.drywall >= required.drywall &&
         available.electrical >= required.electrical &&
         available.plumbing >= required.plumbing &&
         available.hardware >= required.hardware;
}

void addMaterials(ConstructionMaterials &target,
                  const ConstructionMaterials &value) noexcept {
  target.lumber += value.lumber;
  target.drywall += value.drywall;
  target.electrical += value.electrical;
  target.plumbing += value.plumbing;
  target.hardware += value.hardware;
}

void subtractMaterials(ConstructionMaterials &target,
                       const ConstructionMaterials &value) noexcept {
  target.lumber -= value.lumber;
  target.drywall -= value.drywall;
  target.electrical -= value.electrical;
  target.plumbing -= value.plumbing;
  target.hardware -= value.hardware;
}

bool validMaterials(const ConstructionMaterials &value) noexcept {
  constexpr int maximum = 1'000'000;
  return value.lumber >= 0 && value.lumber <= maximum && value.drywall >= 0 &&
         value.drywall <= maximum && value.electrical >= 0 &&
         value.electrical <= maximum && value.plumbing >= 0 &&
         value.plumbing <= maximum && value.hardware >= 0 &&
         value.hardware <= maximum;
}

} // namespace hh::game::detail
