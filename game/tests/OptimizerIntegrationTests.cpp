#include "hh/game/Simulation.h"
#include "StaffOptimization.h"
#include "hh/optimization/Optimizer.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_set>

using namespace hh::game;

namespace {

void require(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}

bool roleEligible(PersonKind kind, TaskKind task) {
  if (task == TaskKind::Turnover || task == TaskKind::Restock)
    return kind == PersonKind::Housekeeper;
  if (task == TaskKind::Repair)
    return kind == PersonKind::Maintenance;
  return kind == PersonKind::Receptionist;
}

void requireAuthorityInvariants(const SimulationView &view) {
  std::unordered_set<EntityId> assignedEmployees;
  for (const auto &task : view.tasks) {
    if (task.status == TaskStatus::Blocked)
      require(task.employeeId == 0, "blocked task retained an employee assignment");
    if (task.status != TaskStatus::Traveling && task.status != TaskStatus::Working)
      continue;
    require(task.employeeId != 0, "active task has no assigned employee");
    const auto person = std::find_if(view.people.begin(), view.people.end(),
                                     [&](const PersonView &candidate) {
                                       return candidate.id == task.employeeId;
                                     });
    require(person != view.people.end(), "task references a missing employee");
    require(person->kind != PersonKind::Guest,
            "guest was assigned to an employee task");
    require(person->onShift, "off-shift employee was assigned work");
    require(roleEligible(person->kind, task.kind),
            "assignment violated role eligibility");
    require(assignedEmployees.insert(person->id).second,
            "employee overlaps multiple live tasks");
  }
}

void native_optimizer_is_byte_deterministic() {
  auto first = Simulation::tutorial(20260909);
  auto second = Simulation::tutorial(20260909);
  first.setStaffOptimizerEnabled(true);
  second.setStaffOptimizerEnabled(true);
  require(first.loadDefinitions(R"({"baseDemand":75})").ok,
          "first deterministic definitions rejected");
  require(second.loadDefinitions(R"({"baseDemand":75})").ok,
          "second deterministic definitions rejected");
  first.step(2 * 86400);
  second.step(2 * 86400);
  require(first.save() == second.save(),
          "same seed/input did not produce byte-identical native-optimizer saves");
}

void optimizer_modes_preserve_authority_invariants() {
  auto legacy = Simulation::tutorial(811);
  auto native = Simulation::tutorial(811);
  legacy.setStaffOptimizerEnabled(false);
  native.setStaffOptimizerEnabled(true);
  require(legacy.loadDefinitions(R"({"baseDemand":90})").ok,
          "legacy-mode definitions rejected");
  require(native.loadDefinitions(R"({"baseDemand":90})").ok,
          "native-mode definitions rejected");

  for (int hour = 0; hour < 48; ++hour) {
    legacy.step(3600);
    native.step(3600);
    requireAuthorityInvariants(legacy.view());
    requireAuthorityInvariants(native.view());
  }
}

void blocked_tasks_are_never_assigned() {
  auto simulation = Simulation::tutorial(812);
  simulation.setStaffOptimizerEnabled(true);
  require(simulation
              .loadDefinitions(
                  R"({"baseDemand":0,"initialLinen":0,"initialTowels":0,"initialAmenities":0,"initialChemicals":0,"initialParts":0})")
              .ok,
          "blocked-task definitions rejected");
  const auto roomId = simulation.view().rooms.front().id;
  require(simulation.requestClean(roomId).ok, "clean request rejected");
  simulation.step(1);

  bool found = false;
  for (const auto &task : simulation.view().tasks)
    if (task.kind == TaskKind::Turnover && task.targetId == roomId &&
        task.status != TaskStatus::Completed) {
      found = true;
      require(task.status == TaskStatus::Blocked,
              "resource-starved turnover did not remain blocked");
      require(task.employeeId == 0, "blocked turnover was assigned");
    }
  require(found, "blocked turnover task was not created");
}

void stale_and_invalid_plans_fall_back_without_mutating_snapshot() {
  hh::optimization::OptimizerSnapshot snapshot;
  snapshot.optimizationEpoch = 17;
  snapshot.simulationSecond = 5100;

  hh::optimization::Employee employee;
  employee.id = 10;
  employee.available = true;
  employee.availableFromBucket = 0;
  employee.availableUntilBucket = hh::optimization::kLivePlanningHorizonBuckets;
  snapshot.employees.push_back(employee);

  hh::optimization::Task task;
  task.id = 20;
  task.state = hh::optimization::TaskState::Ready;
  task.priority = 100;
  task.estimatedWorkSeconds = 60;
  snapshot.tasks.push_back(task);

  hh::optimization::Candidate candidate;
  candidate.employeeId = employee.id;
  candidate.taskId = task.id;
  candidate.eligible = true;
  candidate.travelSeconds = 15;
  candidate.effectiveWorkSeconds = 60;
  snapshot.candidates.push_back(candidate);

  const auto before = hh::optimization::snapshotFingerprint(snapshot);
  hh::optimization::DeterministicFallbackOptimizer optimizer;
  const auto validNative = optimizer.optimize(snapshot);
  require(!validNative.assignments.empty(), "native test plan was empty");

  auto stale = validNative;
  ++stale.optimizationEpoch;
  const auto staleResolution = hh::game::detail::resolveStaffPlan(snapshot, &stale);
  require(staleResolution.valid, "stale plan did not produce a valid fallback");
  require(!staleResolution.acceptedProposed && staleResolution.fellBack,
          "stale plan was accepted instead of falling back");
  require(staleResolution.plan.source ==
              hh::optimization::PlanSource::DeterministicFallback,
          "stale plan did not fall back to native optimizer");
  require(hh::optimization::snapshotFingerprint(snapshot) == before,
          "stale-plan fallback mutated optimizer input state");

  auto invalid = validNative;
  invalid.assignments.front().employeeId = 999999;
  const auto invalidResolution =
      hh::game::detail::resolveStaffPlan(snapshot, &invalid);
  require(invalidResolution.valid,
          "invalid plan did not produce a valid native fallback");
  require(!invalidResolution.acceptedProposed && invalidResolution.fellBack,
          "invalid plan was accepted instead of falling back");
  require(invalidResolution.plan.source ==
              hh::optimization::PlanSource::DeterministicFallback,
          "invalid plan did not resolve through native fallback");
  require(hh::optimization::snapshotFingerprint(snapshot) == before,
          "invalid-plan fallback mutated optimizer input state");
}

void guest_checkout_preempts_older_checkin_backlog() {
  auto simulation = Simulation::tutorial(410);
  require(simulation.loadDefinitions(R"({"baseDemand":100})").ok,
          "optimizer integration definitions rejected");

  EntityId originalReceptionist = 0;
  for (const auto &person : simulation.view().people)
    if (person.kind == PersonKind::Receptionist)
      originalReceptionist = person.id;
  require(originalReceptionist != 0, "tutorial receptionist missing");

  // Allow the arrival wave to reach reception and complete one check-in, then
  // remove reception coverage so older check-in work remains queued.
  simulation.step(3600 + 360);
  require(simulation.fireStaff(originalReceptionist).ok,
          "could not remove original receptionist");

  // Four days guarantees the already-checked-in guest reaches its departure
  // day while the remaining original check-in tasks are still waiting.
  simulation.step(4 * 86400);
  const auto queued = simulation.view();
  bool hasCheckIn = false;
  bool hasCheckOut = false;
  for (const auto &queuedTask : queued.tasks) {
    if (queuedTask.status == TaskStatus::Completed)
      continue;
    hasCheckIn |= queuedTask.kind == TaskKind::CheckIn;
    hasCheckOut |= queuedTask.kind == TaskKind::CheckOut;
  }
  require(hasCheckIn, "test did not produce an older check-in backlog");
  require(hasCheckOut, "test did not produce a checkout task");

  const auto relief = simulation.hireStaff(
      {"Relief", PersonKind::Receptionist, 0, 0, 20});
  require(relief.ok, "could not hire relief receptionist");
  simulation.step(1);

  TaskKind assignedKind = TaskKind::Turnover;
  bool foundAssignment = false;
  for (const auto &assignedTask : simulation.view().tasks)
    if (assignedTask.employeeId == relief.id &&
        assignedTask.status != TaskStatus::Completed) {
      assignedKind = assignedTask.kind;
      foundAssignment = true;
      break;
    }

  require(foundAssignment, "relief receptionist received no queued work");
  require(assignedKind == TaskKind::CheckOut,
          "higher-priority checkout did not preempt older check-in backlog");
}

} // namespace

int main() {
  try {
    native_optimizer_is_byte_deterministic();
    optimizer_modes_preserve_authority_invariants();
    blocked_tasks_are_never_assigned();
    stale_and_invalid_plans_fall_back_without_mutating_snapshot();
    guest_checkout_preempts_older_checkin_backlog();
  } catch (const std::exception &error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
  std::cout << "Optimizer integration tests passed\n";
  return 0;
}
