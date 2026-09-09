from pathlib import Path


def replace_once(path: str, old: str, new: str) -> None:
    target = Path(path)
    text = target.read_text(encoding="utf-8")
    if text.count(old) != 1:
        raise RuntimeError(f"expected exactly one match in {path}, found {text.count(old)}")
    target.write_text(text.replace(old, new, 1), encoding="utf-8")


replace_once(
    "CMakeLists.txt",
    "add_subdirectory(Tools/ContentPipeline)\nadd_subdirectory(game)",
    "add_subdirectory(Tools/ContentPipeline)\nadd_subdirectory(optimization)\nadd_subdirectory(game)",
)

replace_once(
    "game/CMakeLists.txt",
    "add_library(hh_game src/Simulation.cpp)",
    "add_library(hh_game src/Simulation.cpp src/StaffOptimization.cpp)",
)
replace_once(
    "game/CMakeLists.txt",
    "target_link_libraries(hh_game PRIVATE hh_assets)",
    "target_link_libraries(hh_game PRIVATE hh_assets hh_optimization)",
)
replace_once(
    "game/CMakeLists.txt",
    "  target_link_libraries(hh_optimizer_integration_tests PRIVATE hh_game)\n",
    "  target_include_directories(hh_optimizer_integration_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)\n"
    "  target_link_libraries(hh_optimizer_integration_tests PRIVATE hh_game hh_optimization)\n",
)

replace_once(
    "game/include/hh/game/Simulation.h",
    "  CommandResult loadDefinitions(std::string_view jsonText);\n  void step(double seconds);",
    "  CommandResult loadDefinitions(std::string_view jsonText);\n"
    "  void setStaffOptimizerEnabled(bool enabled);\n"
    "  void step(double seconds);",
)

Path("game/src/StaffOptimization.h").write_text(r'''#pragma once

#include "hh/optimization/Types.h"

namespace hh::game::detail {

struct PlanResolution {
  hh::optimization::SchedulerPlan plan;
  bool valid{};
  bool acceptedProposed{};
  bool fellBack{};
};

PlanResolution resolveStaffPlan(
    const hh::optimization::OptimizerSnapshot &snapshot,
    const hh::optimization::SchedulerPlan *proposed = nullptr);

} // namespace hh::game::detail
''', encoding="utf-8")

Path("game/src/StaffOptimization.cpp").write_text(r'''#include "StaffOptimization.h"

#include "hh/optimization/Optimizer.h"
#include "hh/optimization/PlanValidator.h"

namespace hh::game::detail {

PlanResolution resolveStaffPlan(
    const hh::optimization::OptimizerSnapshot &snapshot,
    const hh::optimization::SchedulerPlan *proposed) {
  if (proposed) {
    const auto validation = hh::optimization::validatePlan(snapshot, *proposed);
    if (validation.ok)
      return {*proposed, true, true, false};
  }

  hh::optimization::DeterministicFallbackOptimizer optimizer;
  auto nativePlan = optimizer.optimize(snapshot);
  const auto nativeValidation = hh::optimization::validatePlan(snapshot, nativePlan);
  return {std::move(nativePlan), nativeValidation.ok, false, proposed != nullptr};
}

} // namespace hh::game::detail
''', encoding="utf-8")

replace_once(
    "game/src/Simulation.cpp",
    '#include "hh/assets/Json.h"\n',
    '#include "hh/assets/Json.h"\n#include "StaffOptimization.h"\n#include "hh/optimization/Types.h"\n',
)
replace_once(
    "game/src/Simulation.cpp",
    "  int utilityPerRoomDayCents{350};\n",
    "  int utilityPerRoomDayCents{350};\n  bool staffOptimizerEnabled{true};\n",
)

helpers = r'''  hh::optimization::TaskState optimizerTaskState(const Task &task) const {
    switch (task.status) {
    case TaskStatus::Ready:
      return hh::optimization::TaskState::Ready;
    case TaskStatus::Blocked:
      return hh::optimization::TaskState::Blocked;
    case TaskStatus::Traveling:
      return hh::optimization::TaskState::Traveling;
    case TaskStatus::Working:
      return hh::optimization::TaskState::Executing;
    case TaskStatus::Completed:
      return hh::optimization::TaskState::Completed;
    }
    return hh::optimization::TaskState::Created;
  }
  int optimizerPriority(TaskKind kind) const {
    switch (kind) {
    case TaskKind::CheckOut:
      return 500;
    case TaskKind::CheckIn:
      return 450;
    case TaskKind::Repair:
      return 300;
    case TaskKind::Turnover:
      return 200;
    case TaskKind::Restock:
      return 100;
    }
    return 0;
  }
  int authoritativeRouteSeconds(Position from, Position to) const {
    if (same(from, to))
      return 0;
    const auto route = path(from, to);
    return route.empty() ? -1 : static_cast<int>(route.size());
  }
  int optimizerTravelSeconds(const Person &person, const Task &task) const {
    if (task.kind == TaskKind::Turnover && !task.resourcesClaimed) {
      const auto closet = supply();
      const int toSupply = authoritativeRouteSeconds(person.position, closet);
      const int toTarget = authoritativeRouteSeconds(closet, task.target);
      return toSupply < 0 || toTarget < 0 ? -1 : toSupply + toTarget;
    }
    return authoritativeRouteSeconds(person.position, task.target);
  }
  bool refreshTaskReadiness(Task &task) {
    bool resources = true;
    if (task.kind == TaskKind::Turnover)
      resources = task.resourcesClaimed ||
                  (has(TileKind::SupplyCloset) && inventory.linen >= 1 &&
                   inventory.towels >= 2 && inventory.amenities >= 1 &&
                   inventory.chemicals >= 1);
    if (task.kind == TaskKind::Repair)
      resources = task.resourcesClaimed || inventory.parts >= 1;
    if (task.kind == TaskKind::CheckIn || task.kind == TaskKind::CheckOut)
      resources = has(TileKind::FrontDesk);
    if (!resources) {
      task.status = TaskStatus::Blocked;
      task.blockedReason = "Required local supplies unavailable";
      return false;
    }
    task.blockedReason.clear();
    task.status = TaskStatus::Ready;
    return true;
  }
  void commitStaffAssignment(Task &task, Person &person) {
    task.employeeId = person.id;
    person.task = task.id;
    person.destination =
        (task.kind == TaskKind::Turnover && !task.resourcesClaimed) ? supply()
                                                                   : task.target;
    person.state = PersonState::Traveling;
    task.status = TaskStatus::Traveling;
    if (task.kind == TaskKind::Repair && !task.resourcesClaimed) {
      inventory.parts--;
      task.resourcesClaimed = true;
    }
    if (task.kind == TaskKind::Turnover)
      if (auto *room = getRoom(task.targetId))
        room->status = RoomStatus::Cleaning;
  }
  bool assignNativeGreedy(Task &task) {
    if (task.status != TaskStatus::Ready || !refreshTaskReadiness(task))
      return false;
    Person *best = nullptr;
    int distance = 999999;
    for (auto &person : people)
      if (person.kind != PersonKind::Guest && person.onShift &&
          person.task == 0 && eligible(person, task.kind)) {
        const int candidateDistance = manhattan(person.position, task.target);
        if (candidateDistance < distance) {
          distance = candidateDistance;
          best = &person;
        }
      }
    if (!best)
      return false;
    commitStaffAssignment(task, *best);
    return true;
  }
  hh::optimization::OptimizerSnapshot buildOptimizerSnapshot() const {
    hh::optimization::OptimizerSnapshot snapshot;
    snapshot.optimizationEpoch =
        elapsed < 0 ? 0 : static_cast<std::uint64_t>(elapsed);
    snapshot.simulationSecond = elapsed;

    for (const auto &person : people) {
      if (person.kind == PersonKind::Guest)
        continue;
      hh::optimization::Employee employee;
      employee.id = person.id;
      employee.available = person.onShift && person.task == 0;
      employee.fatigue = static_cast<std::int32_t>(std::lround(person.fatigue));
      employee.regularWageMinorPerHour = person.hourlyWageCents;
      employee.overtimeWageMinorPerHour = person.hourlyWageCents;
      employee.availableFromBucket = 0;
      employee.availableUntilBucket =
          hh::optimization::kLivePlanningHorizonBuckets;
      snapshot.employees.push_back(employee);
    }

    for (const auto &task : tasks) {
      hh::optimization::Task optimizedTask;
      optimizedTask.id = task.id;
      optimizedTask.state = optimizerTaskState(task);
      optimizedTask.priority = optimizerPriority(task.kind);
      optimizedTask.estimatedWorkSeconds = static_cast<std::int32_t>(
          std::ceil(std::max(0.0, task.workRemainingSeconds)));
      optimizedTask.guestImpactPoints =
          task.kind == TaskKind::CheckOut || task.kind == TaskKind::CheckIn
              ? 100
              : task.kind == TaskKind::Turnover ? 60 : 30;
      optimizedTask.revenueImpactPoints =
          task.kind == TaskKind::Turnover ? 80 : task.kind == TaskKind::Repair ? 70 : 40;
      snapshot.tasks.push_back(optimizedTask);

      if (task.status != TaskStatus::Ready)
        continue;
      for (const auto &person : people) {
        if (person.kind == PersonKind::Guest)
          continue;
        hh::optimization::Candidate candidate;
        candidate.employeeId = person.id;
        candidate.taskId = task.id;
        const int travelSeconds = optimizerTravelSeconds(person, task);
        candidate.eligible = person.onShift && person.task == 0 &&
                             eligible(person, task.kind) && travelSeconds >= 0;
        candidate.travelSeconds = std::max(0, travelSeconds);
        double efficiency = 0.75 + 0.75 * person.skill / 100.0;
        if (person.fatigue > 80)
          efficiency /= 1.25;
        else if (person.fatigue > 60)
          efficiency /= 1.10;
        candidate.effectiveWorkSeconds = static_cast<std::int32_t>(std::ceil(
            std::max(0.0, task.workRemainingSeconds) /
            std::max(0.01, efficiency)));
        candidate.fatiguePenaltySeconds =
            static_cast<std::int32_t>(std::lround(person.fatigue));
        candidate.skillBonusSeconds =
            static_cast<std::int32_t>(std::lround(person.skill));
        snapshot.candidates.push_back(candidate);
      }
    }
    return snapshot;
  }
  bool commitOptimizerPlan(const hh::optimization::SchedulerPlan &plan) {
    struct PendingCommit {
      Task *task{};
      Person *person{};
    };
    std::vector<PendingCommit> commits;
    std::unordered_set<EntityId> employees;
    std::unordered_set<EntityId> assignedTasks;
    int repairPartsNeeded = 0;

    for (const auto &assignment : plan.assignments) {
      if (assignment.startBucket != 0)
        continue;
      auto taskIt = std::find_if(tasks.begin(), tasks.end(), [&](Task &task) {
        return task.id == assignment.taskId;
      });
      auto *person = getPerson(assignment.employeeId);
      if (taskIt == tasks.end() || !person || taskIt->status != TaskStatus::Ready ||
          !person->onShift || person->task != 0 ||
          !eligible(*person, taskIt->kind) ||
          optimizerTravelSeconds(*person, *taskIt) < 0 ||
          !employees.insert(person->id).second ||
          !assignedTasks.insert(taskIt->id).second)
        return false;
      if (taskIt->kind == TaskKind::Repair && !taskIt->resourcesClaimed)
        ++repairPartsNeeded;
      commits.push_back({&*taskIt, person});
    }
    if (repairPartsNeeded > inventory.parts)
      return false;
    for (auto &commit : commits)
      commitStaffAssignment(*commit.task, *commit.person);
    return true;
  }
'''
replace_once(
    "game/src/Simulation.cpp",
    "  void staffAndTasks() {\n",
    helpers + "  void staffAndTasks() {\n",
)

old_assignment_block = r'''    for (auto &t : tasks)
      if (t.status == TaskStatus::Ready || t.status == TaskStatus::Blocked) {
        bool resources = true;
        if (t.kind == TaskKind::Turnover)
          resources = t.resourcesClaimed ||
                      (has(TileKind::SupplyCloset) && inventory.linen >= 1 &&
                       inventory.towels >= 2 && inventory.amenities >= 1 &&
                       inventory.chemicals >= 1);
        if (t.kind == TaskKind::Repair)
          resources = t.resourcesClaimed || inventory.parts >= 1;
        if (t.kind == TaskKind::CheckIn || t.kind == TaskKind::CheckOut)
          resources = has(TileKind::FrontDesk);
        if (!resources) {
          t.status = TaskStatus::Blocked;
          t.blockedReason = "Required local supplies unavailable";
          continue;
        }
        t.blockedReason.clear();
        t.status = TaskStatus::Ready;
        Person *best = nullptr;
        int dist = 999999;
        for (auto &p : people)
          if (p.onShift && p.task == 0 && eligible(p, t.kind)) {
            int d = manhattan(p.position, t.target);
            if (d < dist) {
              dist = d;
              best = &p;
            }
          }
        if (best) {
          t.employeeId = best->id;
          best->task = t.id;
          best->destination =
              (t.kind == TaskKind::Turnover && !t.resourcesClaimed) ? supply()
                                                                    : t.target;
          best->state = PersonState::Traveling;
          t.status = TaskStatus::Traveling;
          if (t.kind == TaskKind::Repair && !t.resourcesClaimed) {
            inventory.parts--;
            t.resourcesClaimed = true;
          }
          if (t.kind == TaskKind::Turnover)
            if (auto *r = getRoom(t.targetId))
              r->status = RoomStatus::Cleaning;
        }
      }
'''
new_assignment_block = r'''    for (auto &task : tasks)
      if (task.status == TaskStatus::Ready || task.status == TaskStatus::Blocked)
        refreshTaskReadiness(task);

    // Guest-facing front-desk work stays entirely native and immediate. A
    // checkout has priority over check-in backlog so departures cannot be
    // starved by older arrivals.
    for (auto &task : tasks)
      if (task.status == TaskStatus::Ready && task.kind == TaskKind::CheckOut)
        assignNativeGreedy(task);
    for (auto &task : tasks)
      if (task.status == TaskStatus::Ready && task.kind == TaskKind::CheckIn)
        assignNativeGreedy(task);

    bool optimizerCommitted = false;
    if (staffOptimizerEnabled) {
      const auto snapshot = buildOptimizerSnapshot();
      const auto resolution = detail::resolveStaffPlan(snapshot);
      optimizerCommitted = resolution.valid && commitOptimizerPlan(resolution.plan);
    }
    if (!staffOptimizerEnabled || !optimizerCommitted)
      for (auto &task : tasks)
        if (task.status == TaskStatus::Ready)
          assignNativeGreedy(task);
'''
replace_once("game/src/Simulation.cpp", old_assignment_block, new_assignment_block)

replace_once(
    "game/src/Simulation.cpp",
    "  *impl_ = std::move(d);\n  return {true, \"Definitions loaded\"};\n}\nvoid Simulation::step(double seconds) {",
    "  *impl_ = std::move(d);\n  return {true, \"Definitions loaded\"};\n}\n"
    "void Simulation::setStaffOptimizerEnabled(bool enabled) {\n"
    "  impl_->staffOptimizerEnabled = enabled;\n"
    "}\n"
    "void Simulation::step(double seconds) {",
)

print("Applied Hotel Haven NVIDIA integration bridge")
