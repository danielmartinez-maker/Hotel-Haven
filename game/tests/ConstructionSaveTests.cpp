#include "hh/game/BuildJobs.h"
#include "hh/game/BuildingSystems.h"
#include "hh/game/Construction.h"
#include "hh/game/Simulation.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>

using namespace hh::game;

namespace {
void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

void floorGrid(Simulation &sim) {
  for (int y = 0; y < 8; ++y)
    for (int x = 0; x < 12; ++x)
      require(sim.buildTile({0, x, y}, x == 0 && y == 0
                                           ? TileKind::Entrance
                                           : TileKind::Floor)
                  .ok,
              "save fixture support floor failed");
}

void complete_construction_state_round_trips_and_migrates_v10() {
  Simulation sim(4001, 12, 8, 3);
  floorGrid(sim);
  ConstructionCommand placed;
  placed.placements.push_back({"chair", {0, 3, 3}, 0});
  require(sim.executeConstruction(placed).ok,
          "save fixture object placement failed");
  require(sim.addConstructionMaterials({5, 5, 5, 5, 5}).ok,
          "save fixture material stocking failed");
  BuildPlan queuedPlan;
  queuedPlan.construction.placements.push_back({"desk", {0, 6, 3}, 0});
  require(sim.queueBuild(queuedPlan).ok, "save fixture build job failed");

  ElevatorSpec elevator;
  elevator.kind = ElevatorKind::Service;
  elevator.minFloor = 0;
  elevator.maxFloor = 2;
  elevator.startFloor = 1;
  const auto installed = sim.installElevator(elevator);
  require(installed.ok && sim.requestElevator(installed.id, 1, 2).ok,
          "save fixture elevator failed");
  sim.step(1);

  const auto saved = sim.save();
  require(saved.starts_with("HHGS 11 "),
          "FINAL-01 state did not bump the save format to v11");
  auto loaded = Simulation::load(saved);
  require(loaded.save() == saved,
          "v11 construction/building state was not byte-stable after load");
  require(loaded.constructionSnapshot() == sim.constructionSnapshot(),
          "construction state did not round-trip");
  require(loaded.buildingSystemsSnapshot() == sim.buildingSystemsSnapshot(),
          "building-system state did not round-trip");

  Simulation empty(4002, 4, 4, 1);
  auto legacy = empty.save();
  const auto marker = legacy.find("HHGS 11 ");
  require(marker == 0, "v11 migration fixture header missing");
  legacy.replace(5, 2, "10");
  auto migrated = Simulation::load(legacy);
  require(migrated.save().starts_with("HHGS 11 ") &&
              migrated.constructionSnapshot().objects.empty() &&
              migrated.constructionSnapshot().buildJobs.empty() &&
              migrated.buildingSystemsSnapshot().elevators.empty(),
          "v10 save did not migrate with safe FINAL-01 defaults");
}

void batched_elevator_mid_trip_save_continues_identically() {
  Simulation direct(4003, 8, 8, 8);
  ElevatorSpec spec;
  spec.kind = ElevatorKind::Passenger;
  spec.minFloor = 0;
  spec.maxFloor = 7;
  spec.startFloor = 0;
  spec.capacity = 2;
  spec.travelSecondsPerFloor = 3;
  spec.doorSeconds = 2;
  const auto installed = direct.installElevator(spec);
  require(installed.ok, "mid-trip elevator install failed");
  require(direct.requestElevator(ElevatorKind::Passenger, 0, 6).ok &&
              direct.requestElevator(ElevatorKind::Passenger, 0, 4).ok,
          "mid-trip bank requests failed");
  direct.step(7);
  const auto before = direct.buildingSystemsSnapshot().elevators.front();
  const auto boarded = std::count_if(
      before.requests.begin(), before.requests.end(),
      [](const ElevatorRequestSnapshot &request) { return request.boarded; });
  require(boarded == 2 && before.state == ElevatorState::MovingToDestination,
          "mid-trip fixture did not reach a batched moving state");

  auto resumed = Simulation::load(direct.save());
  direct.step(90);
  resumed.step(90);
  require(resumed.save() == direct.save(),
          "batched elevator continuation diverged after save/load");
  require(resumed.buildingSystemsSnapshot() == direct.buildingSystemsSnapshot(),
          "batched elevator authoritative state diverged after save/load");
}
} // namespace

int main() {
  try {
    complete_construction_state_round_trips_and_migrates_v10();
    batched_elevator_mid_trip_save_continues_identically();
  } catch (const std::exception &error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
  std::cout << "All construction save tests passed\n";
  return 0;
}
