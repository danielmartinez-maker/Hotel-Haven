from pathlib import Path

path = Path("game/src/Simulation.cpp")
text = path.read_text(encoding="utf-8")

include_old = '#include "hh/game/Simulation.h"\n#include "hh/assets/Json.h"\n'
include_new = '#include "hh/game/Simulation.h"\n#include "hh/assets/Json.h"\n#include "hh/optimization/Optimizer.h"\n#include "hh/optimization/PlanValidator.h"\n'
if text.count(include_old) != 1:
    raise SystemExit("expected Simulation include seam exactly once")
text = text.replace(include_old, include_new, 1)

post_wage = '''  void postAccruedWage(Person &person) {
    const std::int64_t cents = person.accruedWageUnits / 3600;
    person.accruedWageUnits %= 3600;
    economy.payrollCents += cents;
    economy.cashCents -= cents;
  }
'''
helpers = post_wage + '''  static hh::optimization::TaskState optimizerTaskState(TaskStatus status) {
    switch (status) {
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
    return hh::optimization::TaskState::Failed;
  }
  static int optimizerPriority(TaskKind kind) {
    switch (kind) {
    case TaskKind::CheckOut:
      return 90;
    case TaskKind::CheckIn:
      return 85;
    case TaskKind::Repair:
      return 60;
    case TaskKind::Turnover:
      return 50;
    case TaskKind::Restock:
      return 35;
    }
    return 0;
  }
  Position assignmentDestination(const Task &task) const {
    return task.kind == TaskKind::Turnover && !task.resourcesClaimed ? supply()
                                                                    : task.target;
  }
  int effectiveWorkSeconds(const Person &person, const Task &task) const {
    double efficiency = 0.75 + 0.75 * person.skill / 100.0;
    if (person.fatigue > 80)
      efficiency /= 1.25;
    else if (person.fatigue > 60)
      efficiency /= 1.10;
    return static_cast<int>(std::ceil(
        std::max(0.0, task.workRemainingSeconds) / std::max(0.01, efficiency)));
  }
  hh::optimization::OptimizerSnapshot buildOptimizerSnapshot() const {
    hh::optimization::OptimizerSnapshot snapshot;
    snapshot.optimizationEpoch = static_cast<std::uint64_t>(elapsed);
    snapshot.simulationSecond = elapsed;
    for (const auto &person : people)
      if (person.kind != PersonKind::Guest) {
        hh::optimization::Employee employee;
        employee.id = person.id;
        employee.available = person.onShift && person.task == 0 &&
                             person.state == PersonState::Idle;
        employee.fatigue = static_cast<std::int32_t>(
            std::lround(std::clamp(person.fatigue, 0.0, 100.0)));
        employee.regularWageMinorPerHour = person.hourlyWageCents;
        employee.overtimeWageMinorPerHour = person.hourlyWageCents;
        employee.availableFromBucket = 0;
        employee.availableUntilBucket = std::numeric_limits<std::int32_t>::max();
        snapshot.employees.push_back(employee);
      }
    for (const auto &task : tasks) {
      hh::optimization::Task optimizerTask;
      optimizerTask.id = task.id;
      optimizerTask.state = optimizerTaskState(task.status);
      optimizerTask.priority = optimizerPriority(task.kind);
      optimizerTask.estimatedWorkSeconds =
          static_cast<std::int32_t>(std::ceil(std::max(0.0, task.workRemainingSeconds)));
      optimizerTask.guestImpactPoints =
          task.kind == TaskKind::CheckIn || task.kind == TaskKind::CheckOut ? 30
          : task.kind == TaskKind::Turnover                            ? 15
                                                                       : 10;
      optimizerTask.revenueImpactPoints =
          task.kind == TaskKind::CheckOut ? 25
          : task.kind == TaskKind::CheckIn ? 15
          : task.kind == TaskKind::Turnover ? 10
                                             : 5;
      snapshot.tasks.push_back(optimizerTask);
    }
    for (const auto &task : tasks)
      if (task.status == TaskStatus::Ready)
        for (const auto &person : people)
          if (person.kind != PersonKind::Guest) {
            hh::optimization::Candidate candidate;
            candidate.employeeId = person.id;
            candidate.taskId = task.id;
            candidate.eligible = person.onShift && person.task == 0 &&
                                 person.state == PersonState::Idle &&
                                 eligible(person, task.kind);
            if (candidate.eligible) {
              const auto destination = assignmentDestination(task);
              const auto route = path(person.position, destination);
              if (route.empty() && !same(person.position, destination)) {
                candidate.eligible = false;
              } else {
                candidate.travelSeconds = static_cast<std::int32_t>(route.size());
                candidate.effectiveWorkSeconds = effectiveWorkSeconds(person, task);
                candidate.fatiguePenaltySeconds = static_cast<std::int32_t>(
                    std::ceil(std::clamp(person.fatigue, 0.0, 100.0) * 2.0));
              }
            }
            snapshot.candidates.push_back(candidate);
          }
    return snapshot;
  }
  bool commitAssignment(Task &task, Person &person) {
    if (task.status != TaskStatus::Ready || !person.onShift || person.task != 0 ||
        person.state != PersonState::Idle || !eligible(person, task.kind))
      return false;
    if (task.kind == TaskKind::Repair && !task.resourcesClaimed &&
        inventory.parts < 1)
      return false;
    const auto destination = assignmentDestination(task);
    if (!same(person.position, destination) && path(person.position, destination).empty())
      return false;
    task.employeeId = person.id;
    person.task = task.id;
    person.destination = destination;
    person.state = PersonState::Traveling;
    task.status = TaskStatus::Traveling;
    if (task.kind == TaskKind::Repair && !task.resourcesClaimed) {
      inventory.parts--;
      task.resourcesClaimed = true;
    }
    if (task.kind == TaskKind::Turnover)
      if (auto *room = getRoom(task.targetId))
        room->status = RoomStatus::Cleaning;
    return true;
  }
  void legacyAssignReadyTasks() {
    for (auto &task : tasks)
      if (task.status == TaskStatus::Ready) {
        Person *best = nullptr;
        int distance = std::numeric_limits<int>::max();
        for (auto &person : people)
          if (person.onShift && person.task == 0 && person.state == PersonState::Idle &&
              eligible(person, task.kind)) {
            const auto destination = assignmentDestination(task);
            const auto route = path(person.position, destination);
            if (route.empty() && !same(person.position, destination))
              continue;
            const int candidateDistance = static_cast<int>(route.size());
            if (candidateDistance < distance ||
                (candidateDistance == distance &&
                 (!best || person.id < best->id))) {
              distance = candidateDistance;
              best = &person;
            }
          }
        if (best)
          (void)commitAssignment(task, *best);
      }
  }
  void assignReadyTasksWithOptimizer() {
    const auto snapshot = buildOptimizerSnapshot();
    hh::optimization::DeterministicFallbackOptimizer optimizer;
    const auto plan = optimizer.optimize(snapshot);
    const auto validation = hh::optimization::validatePlan(snapshot, plan);
    if (!validation.ok) {
      legacyAssignReadyTasks();
      return;
    }
    for (const auto &assignment : plan.assignments) {
      auto taskIt = std::find_if(tasks.begin(), tasks.end(), [&](const Task &task) {
        return task.id == assignment.taskId;
      });
      auto personIt = std::find_if(people.begin(), people.end(), [&](const Person &person) {
        return person.id == assignment.employeeId;
      });
      if (taskIt != tasks.end() && personIt != people.end())
        (void)commitAssignment(*taskIt, *personIt);
    }
  }
'''
if text.count(post_wage) != 1:
    raise SystemExit("expected postAccruedWage seam exactly once")
text = text.replace(post_wage, helpers, 1)

old_assignment = '''    for (auto &t : tasks)
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
new_assignment = '''    for (auto &t : tasks)
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
      }
    assignReadyTasksWithOptimizer();
'''
if text.count(old_assignment) != 1:
    raise SystemExit("expected legacy assignment seam exactly once")
text = text.replace(old_assignment, new_assignment, 1)

path.write_text(text, encoding="utf-8")
print("Patched authoritative scheduler assignment seam")
