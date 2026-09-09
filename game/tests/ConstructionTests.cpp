#include "hh/game/BuildJobs.h"
#include "hh/game/Construction.h"
#include "hh/game/Simulation.h"
#include <iostream>
#include <stdexcept>
#include <string>

using namespace hh::game;

namespace {
void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

void floorGrid(Simulation &sim, int width, int height) {
  for (int y = 0; y < height; ++y)
    for (int x = 0; x < width; ++x) {
      const auto kind = x == 0 && y == 0 ? TileKind::Entrance : TileKind::Floor;
      require(sim.buildTile({0, x, y}, kind).ok,
              "construction fixture could not build support floor");
    }
}

ConstructionCommand placement(std::string type, Position origin,
                              int rotationQuarterTurns = 0) {
  ConstructionCommand command;
  command.placements.push_back(
      {std::move(type), origin, rotationQuarterTurns});
  return command;
}

void construction_rejects_overlap_invalid_support_and_unaffordable_commands() {
  Simulation sim(1001, 20, 12, 1);
  floorGrid(sim, 20, 12);

  ConstructionCommand furnishing;
  furnishing.placements.push_back({"desk", {0, 3, 3}, 0});
  furnishing.placements.push_back({"chair", {0, 6, 3}, 0});
  const auto placed = sim.executeConstruction(furnishing);
  require(placed.ok, "valid furnishing placement was rejected");

  const auto overlap = sim.previewConstruction(
      placement("guest_bed", {0, 3, 3}));
  require(!overlap.valid &&
              overlap.reason == ConstructionReason::OccupiedFootprint,
          "overlapping object placement did not expose OccupiedFootprint");

  const auto unsupported = sim.previewConstruction(
      placement("wall_sconce", {0, 10, 4}));
  require(!unsupported.valid &&
              unsupported.reason == ConstructionReason::InvalidSupport,
          "wall fixture on floor did not expose InvalidSupport");

  Simulation poor(1002, 64, 16, 1);
  floorGrid(poor, 64, 16);
  ConstructionCommand tooLarge;
  for (int y = 0; y < 16 && tooLarge.placements.size() < 300; ++y)
    for (int x = 1; x < 64 && tooLarge.placements.size() < 300; x += 2)
      tooLarge.placements.push_back({"chair", {0, x, y}, 0});
  const auto unaffordable = poor.previewConstruction(tooLarge);
  require(!unaffordable.valid &&
              unaffordable.reason == ConstructionReason::InsufficientCash,
          "unaffordable object command did not expose InsufficientCash");
}

void validated_multi_object_construction_commits_atomically() {
  Simulation sim(1003, 20, 12, 1);
  floorGrid(sim, 20, 12);
  const auto before = sim.view().economy.cashCents;
  const auto objectsBefore = sim.constructionSnapshot().objects.size();

  ConstructionCommand command;
  command.placements.push_back({"desk", {0, 3, 3}, 0});
  command.placements.push_back({"chair", {0, 6, 3}, 0});
  const auto preview = sim.previewConstruction(command);
  require(preview.valid && preview.costCents > 0,
          "valid multi-object command did not produce a priced preview");
  const auto result = sim.executeConstruction(command);
  require(result.ok && result.costCents == preview.costCents,
          "validated construction did not commit at previewed exact-cent cost");
  require(sim.view().economy.cashCents == before - result.costCents,
          "construction did not debit the exact previewed cost");
  require(sim.constructionSnapshot().objects.size() == objectsBefore + 2,
          "multi-object construction did not create both objects");

  ConstructionCommand invalidAtomic;
  invalidAtomic.placements.push_back({"chair", {0, 9, 3}, 0});
  invalidAtomic.placements.push_back({"wall_sconce", {0, 10, 4}, 0});
  const auto cashBefore = sim.view().economy.cashCents;
  const auto countBefore = sim.constructionSnapshot().objects.size();
  const auto rejected = sim.executeConstruction(invalidAtomic);
  require(!rejected.ok &&
              rejected.error == ConstructionReason::InvalidSupport,
          "invalid atomic construction did not return the support error");
  require(sim.view().economy.cashCents == cashBefore &&
              sim.constructionSnapshot().objects.size() == countBefore,
          "rejected multi-object construction partially mutated authority");
}
} // namespace

int main() {
  try {
    construction_rejects_overlap_invalid_support_and_unaffordable_commands();
    validated_multi_object_construction_commits_atomically();
  } catch (const std::exception &error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
  std::cout << "All construction tests passed\n";
  return 0;
}
