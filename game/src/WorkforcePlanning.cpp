#include "hh/game/Simulation.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace hh::game {
namespace {

constexpr std::int64_t SecondsPerHour = 60 * 60;
constexpr std::int64_t SecondsPerDay = 24 * SecondsPerHour;
constexpr std::int64_t MaxPlanningHorizon = 30 * SecondsPerDay;

StaffRole staffRole(PersonKind kind) {
  switch (kind) {
  case PersonKind::Receptionist:
    return StaffRole::Receptionist;
  case PersonKind::Housekeeper:
    return StaffRole::Housekeeper;
  case PersonKind::Maintenance:
    return StaffRole::Maintenance;
  case PersonKind::Guest:
    break;
  }
  throw std::invalid_argument("guest has no staff role");
}

StaffRole requiredRole(TaskKind kind) {
  switch (kind) {
  case TaskKind::Turnover:
  case TaskKind::Restock:
    return StaffRole::Housekeeper;
  case TaskKind::Repair:
    return StaffRole::Maintenance;
  case TaskKind::CheckIn:
  case TaskKind::CheckOut:
    return StaffRole::Receptionist;
  }
  throw std::invalid_argument("unknown task kind");
}

bool criticalTask(TaskKind kind) {
  return kind == TaskKind::Repair || kind == TaskKind::CheckIn ||
         kind == TaskKind::CheckOut;
}

std::vector<OptimizationWindow>
shiftWindows(const PersonView &person, std::int64_t captured,
             std::int64_t horizonEnd) {
  std::vector<OptimizationWindow> windows;
  const auto firstDay = captured / SecondsPerDay - 1;
  const auto lastDay = horizonEnd / SecondsPerDay + 1;

  for (auto day = firstDay; day <= lastDay; ++day) {
    const auto dayStart = day * SecondsPerDay;
    std::int64_t start{};
    std::int64_t end{};
    if (person.shiftStartHour == person.shiftEndHour) {
      start = dayStart;
      end = dayStart + SecondsPerDay;
    } else if (person.shiftStartHour < person.shiftEndHour) {
      start = dayStart + person.shiftStartHour * SecondsPerHour;
      end = dayStart + person.shiftEndHour * SecondsPerHour;
    } else {
      start = dayStart + person.shiftStartHour * SecondsPerHour;
      end = dayStart + SecondsPerDay +
            person.shiftEndHour * SecondsPerHour;
    }

    start = std::max(start, captured);
    end = std::min(end, horizonEnd);
    if (start < end)
      windows.push_back({start, end});
  }

  std::sort(windows.begin(), windows.end(), [](const auto &a, const auto &b) {
    if (a.startSecond != b.startSecond)
      return a.startSecond < b.startSecond;
    return a.endSecond < b.endSecond;
  });
  std::vector<OptimizationWindow> merged;
  for (const auto &window : windows) {
    if (!merged.empty() &&
        window.startSecond <= merged.back().endSecond) {
      merged.back().endSecond =
          std::max(merged.back().endSecond, window.endSecond);
    } else {
      merged.push_back(window);
    }
  }
  return merged;
}

bool containsNow(const std::vector<OptimizationWindow> &windows,
                 std::int64_t now) {
  return std::any_of(windows.begin(), windows.end(), [&](const auto &window) {
    return now >= window.startSecond && now < window.endSecond;
  });
}

std::int64_t taskBusyUntil(const PersonView &person, const TaskView &task,
                           std::int64_t captured,
                           std::int64_t horizonEnd) {
  const auto dx = std::abs(person.position.x - task.target.x);
  const auto dy = std::abs(person.position.y - task.target.y);
  const auto df = std::abs(person.position.floor - task.target.floor);
  const auto travel = static_cast<std::int64_t>(dx + dy + df);
  const auto work = static_cast<std::int64_t>(
      std::ceil(std::max(0.0, task.workRemainingSeconds)));
  if (travel > horizonEnd - captured)
    return horizonEnd;
  const auto afterTravel = captured + travel;
  if (work > horizonEnd - afterTravel)
    return horizonEnd;
  return std::min(horizonEnd, afterTravel + work);
}

} // namespace

OptimizerSnapshot
Simulation::buildOptimizerSnapshot(std::int64_t horizonSeconds) const {
  if (horizonSeconds <= 0 || horizonSeconds > MaxPlanningHorizon)
    throw std::invalid_argument(
        "planning horizon must be between one second and 30 days");

  const auto current = view();
  if (current.elapsedSeconds >
      std::numeric_limits<std::int64_t>::max() - horizonSeconds)
    throw std::overflow_error("planning horizon overflows simulation clock");

  OptimizerSnapshot snapshot;
  snapshot.capturedSecond = current.elapsedSeconds;
  snapshot.horizonEndSecond = current.elapsedSeconds + horizonSeconds;

  snapshot.employees.reserve(current.people.size());
  for (const auto &person : current.people) {
    if (person.kind == PersonKind::Guest)
      continue;

    OptimizerEmployee employee;
    employee.id = person.id;
    employee.role = staffRole(person.kind);
    employee.shiftWindows =
        shiftWindows(person, snapshot.capturedSecond, snapshot.horizonEndSecond);

    for (const auto &task : current.tasks) {
      if (task.employeeId != person.id ||
          (task.status != TaskStatus::Traveling &&
           task.status != TaskStatus::Working))
        continue;
      const auto busyUntil =
          taskBusyUntil(person, task, snapshot.capturedSecond,
                        snapshot.horizonEndSecond);
      if (busyUntil > snapshot.capturedSecond)
        employee.unavailableWindows.push_back(
            {snapshot.capturedSecond, busyUntil});
    }

    employee.availableNow =
        containsNow(employee.shiftWindows, snapshot.capturedSecond) &&
        employee.unavailableWindows.empty();
    snapshot.employees.push_back(std::move(employee));
  }

  snapshot.tasks.reserve(current.tasks.size());
  for (const auto &task : current.tasks) {
    if (task.status != TaskStatus::Ready || task.employeeId != 0)
      continue;
    const auto duration = static_cast<std::int64_t>(
        std::ceil(std::max(1.0, task.workRemainingSeconds)));
    OptimizerTask planned;
    planned.id = task.id;
    planned.requiredRole = requiredRole(task.kind);
    planned.critical = criticalTask(task.kind);
    planned.earliestStartSecond = snapshot.capturedSecond;
    planned.durationSeconds = static_cast<int>(
        std::min<std::int64_t>(duration, std::numeric_limits<int>::max()));
    snapshot.tasks.push_back(planned);
  }

  return snapshot;
}

PlanValidation Simulation::validatePlan(const OptimizerSnapshot &snapshot,
                                        const AssignmentPlan &plan) const {
  return validateAssignmentPlan(snapshot, plan);
}

} // namespace hh::game
