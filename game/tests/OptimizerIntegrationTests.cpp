#include "hh/game/Simulation.h"
#include "hh/game/StaffOptimization.h"

#include <iostream>
#include <stdexcept>

using namespace hh::game;

static void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

static OptimizerSnapshot validation_snapshot() {
  OptimizerSnapshot snapshot;
  snapshot.capturedSecond = 100;
  snapshot.horizonEndSecond = 1000;
  snapshot.employees = {
      {1, StaffRole::Housekeeper, true, false, {{100, 500}}, {}},
      {2, StaffRole::Housekeeper, false, true, {{100, 500}}, {{200, 260}}},
  };
  snapshot.tasks = {{11, StaffRole::Housekeeper, true, 100, 60}};
  return snapshot;
}

static void optimizer_rejects_absence_and_break_overlap() {
  Simulation sim(401, 8, 8, 1);
  const auto snapshot = validation_snapshot();

  AssignmentPlan absent;
  absent.assignments.push_back({11, 1, 100, 160});
  require(!sim.validatePlan(snapshot, absent).ok,
          "optimizer accepted absent employee");

  AssignmentPlan breakOverlap;
  breakOverlap.assignments.push_back({11, 2, 210, 270});
  require(!sim.validatePlan(snapshot, breakOverlap).ok,
          "optimizer accepted break overlap");

  AssignmentPlan valid;
  valid.assignments.push_back({11, 2, 100, 160});
  require(sim.validatePlan(snapshot, valid).ok,
          "optimizer rejected valid assignment");
}

static void deterministic_fallback_returns_same_plan() {
  auto snapshot = validation_snapshot();
  snapshot.employees[0].absent = false;
  snapshot.employees[0].availableNow = true;
  snapshot.tasks.push_back({12, StaffRole::Housekeeper, false, 100, 45});

  const auto first = buildDeterministicFallbackPlan(snapshot);
  const auto second = buildDeterministicFallbackPlan(snapshot);
  require(first == second, "same snapshot produced different fallback plan");
  require(!first.assignments.empty(), "fallback plan assigned no work");
  require(first.assignments.front().taskId == 11,
          "critical task was not planned first");
  require(validateAssignmentPlan(snapshot, first).ok,
          "native fallback generated an invalid plan");
}

static void simulation_snapshot_contains_workforce_constraints() {
  Simulation sim(402, 8, 8, 1);
  require(sim.buildTile({0, 0, 0}, TileKind::Entrance).ok,
          "optimizer snapshot entrance failed");
  const auto hire =
      sim.hireStaff({"Planner", PersonKind::Housekeeper, 0, 0, 18});
  require(hire.ok, "optimizer snapshot hire failed");
  require(sim.loadDefinitions(
                 R"({"staffBreakAfterMinutes":1,"staffBreakDurationMinutes":1})")
              .ok,
          "optimizer snapshot break definitions rejected");
  sim.step(61);

  const auto snapshot = sim.buildOptimizerSnapshot();
  require(snapshot.capturedSecond == sim.view().elapsedSeconds,
          "optimizer snapshot timestamp mismatch");
  bool found = false;
  for (const auto &employee : snapshot.employees)
    if (employee.id == hire.id) {
      found = true;
      require(!employee.shiftWindows.empty(),
              "optimizer snapshot omitted shift availability");
      require(!employee.unavailableWindows.empty(),
              "optimizer snapshot omitted break constraint");
    }
  require(found, "optimizer snapshot omitted employee");
}

static void native_scheduler_completes_critical_work_without_optimizer() {
  auto sim = Simulation::tutorial(403);
  require(sim.loadDefinitions(R"({"repairWorkSeconds":5})").ok,
          "fallback repair definitions rejected");
  const auto maintenance =
      sim.hireStaff({"Fallback Tech", PersonKind::Maintenance, 0, 0, 25});
  require(maintenance.ok, "fallback maintenance hire failed");
  const auto roomId = sim.view().rooms.front().id;
  require(sim.requestRepair(roomId).ok, "fallback repair request rejected");
  sim.step(120);

  bool activeRepair = false;
  for (const auto &task : sim.view().tasks)
    if (task.targetId == roomId && task.kind == TaskKind::Repair &&
        task.status != TaskStatus::Completed)
      activeRepair = true;
  require(!activeRepair,
          "native scheduler failed critical repair without optimizer");
}

int main() {
  try {
    optimizer_rejects_absence_and_break_overlap();
    deterministic_fallback_returns_same_plan();
    simulation_snapshot_contains_workforce_constraints();
    native_scheduler_completes_critical_work_without_optimizer();
  } catch (const std::exception &e) {
    std::cerr << "FAIL: " << e.what() << '\n';
    return 1;
  }
  std::cout << "Optimizer integration tests passed\n";
  return 0;
}
