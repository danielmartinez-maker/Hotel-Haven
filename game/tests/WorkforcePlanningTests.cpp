#include "hh/game/Simulation.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>

using namespace hh::game;

static void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

static Simulation scenario(EntityId &housekeeperId, EntityId &roomId) {
  Simulation sim(510, 16, 12, 1);
  for (int x = 0; x <= 8; ++x)
    require(sim.buildTile({0, x, 0},
                          x == 0 ? TileKind::Entrance : TileKind::Floor)
                .ok,
            "planning corridor failed");
  require(sim.buildTile({0, 2, 0}, TileKind::SupplyCloset).ok,
          "planning supply closet failed");
  const auto room = sim.buildFurnishedRoom(
      {"101", 0, 4, 1, 4, 4, {0, 4, 1}, 1, 1, 120});
  require(room.ok, "planning room failed");
  const auto housekeeper =
      sim.hireStaff({"Planner HK", PersonKind::Housekeeper, 0, 0, 18});
  const auto receptionist =
      sim.hireStaff({"Planner FO", PersonKind::Receptionist, 0, 0, 20});
  const auto maintenance =
      sim.hireStaff({"Planner ENG", PersonKind::Maintenance, 0, 0, 24});
  require(housekeeper.ok && receptionist.ok && maintenance.ok,
          "planning staff hire failed");
  require(sim.requestClean(room.id).ok, "planning clean request failed");
  housekeeperId = housekeeper.id;
  roomId = room.id;
  return sim;
}

int main() {
  try {
    EntityId housekeeperId{}, roomId{};
    auto sim = scenario(housekeeperId, roomId);
    const auto before = sim.save();

    const auto snapshot = sim.buildOptimizerSnapshot(3600);
    require(sim.save() == before, "optimizer snapshot mutated simulation");
    require(snapshot.capturedSecond == sim.view().elapsedSeconds,
            "snapshot timestamp mismatch");
    require(snapshot.horizonEndSecond - snapshot.capturedSecond == 3600,
            "snapshot horizon mismatch");
    require(snapshot.employees.size() == 3,
            "snapshot omitted staff");

    const auto hk = std::find_if(
        snapshot.employees.begin(), snapshot.employees.end(),
        [&](const auto &employee) { return employee.id == housekeeperId; });
    require(hk != snapshot.employees.end(), "housekeeper missing");
    require(hk->role == StaffRole::Housekeeper,
            "housekeeper role mapping failed");
    require(hk->availableNow, "24-hour idle housekeeper not available");
    require(!hk->shiftWindows.empty(), "shift windows missing");

    const auto task = std::find_if(
        snapshot.tasks.begin(), snapshot.tasks.end(),
        [&](const auto &candidate) {
          return candidate.requiredRole == StaffRole::Housekeeper;
        });
    require(task != snapshot.tasks.end(), "ready turnover task missing");
    require(task->durationSeconds > 0, "task duration not bounded");

    const auto plan = buildDeterministicFallbackPlan(snapshot);
    require(!plan.assignments.empty(), "fallback planner assigned no work");
    require(sim.validatePlan(snapshot, plan).ok,
            "simulation rejected deterministic fallback plan");
    require(sim.save() == before, "plan validation mutated simulation");

    bool rejected = false;
    try {
      (void)sim.buildOptimizerSnapshot(0);
    } catch (const std::invalid_argument &) {
      rejected = true;
    }
    require(rejected, "zero planning horizon was accepted");
  } catch (const std::exception &e) {
    std::cerr << "FAIL: " << e.what() << '\n';
    return 1;
  }
  std::cout << "Workforce planning tests passed\n";
  return 0;
}
