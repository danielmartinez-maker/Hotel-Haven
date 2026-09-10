#include "hh/game/StaffOptimization.h"

#include <algorithm>
#include <limits>
#include <unordered_map>
#include <unordered_set>

namespace hh::game {
namespace {

bool overlaps(const OptimizationWindow &a, const OptimizationWindow &b) {
  return a.startSecond < b.endSecond && b.startSecond < a.endSecond;
}

bool contains(const OptimizationWindow &window, std::int64_t start,
              std::int64_t end) {
  return start >= window.startSecond && end <= window.endSecond;
}

const OptimizerEmployee *employeeById(const OptimizerSnapshot &snapshot,
                                      std::uint64_t id) {
  const auto it = std::find_if(snapshot.employees.begin(), snapshot.employees.end(),
                               [&](const OptimizerEmployee &employee) {
                                 return employee.id == id;
                               });
  return it == snapshot.employees.end() ? nullptr : &*it;
}

const OptimizerTask *taskById(const OptimizerSnapshot &snapshot,
                              std::uint64_t id) {
  const auto it = std::find_if(snapshot.tasks.begin(), snapshot.tasks.end(),
                               [&](const OptimizerTask &task) {
                                 return task.id == id;
                               });
  return it == snapshot.tasks.end() ? nullptr : &*it;
}

std::int64_t earliestFit(const OptimizerSnapshot &snapshot,
                         const OptimizerEmployee &employee,
                         const OptimizerTask &task,
                         const std::vector<Assignment> &planned) {
  std::vector<OptimizationWindow> blockers = employee.unavailableWindows;
  for (const auto &assignment : planned)
    if (assignment.employeeId == employee.id)
      blockers.push_back({assignment.startSecond, assignment.endSecond});
  std::sort(blockers.begin(), blockers.end(), [](const auto &a, const auto &b) {
    if (a.startSecond != b.startSecond)
      return a.startSecond < b.startSecond;
    return a.endSecond < b.endSecond;
  });

  auto shifts = employee.shiftWindows;
  std::sort(shifts.begin(), shifts.end(), [](const auto &a, const auto &b) {
    if (a.startSecond != b.startSecond)
      return a.startSecond < b.startSecond;
    return a.endSecond < b.endSecond;
  });
  for (const auto &shift : shifts) {
    auto cursor = std::max({snapshot.capturedSecond, task.earliestStartSecond,
                            shift.startSecond});
    if (cursor + task.durationSeconds > shift.endSecond)
      continue;
    for (const auto &blocker : blockers) {
      if (blocker.endSecond <= cursor || blocker.startSecond >= shift.endSecond)
        continue;
      if (cursor + task.durationSeconds <= blocker.startSecond)
        return cursor;
      cursor = std::max(cursor, blocker.endSecond);
      if (cursor + task.durationSeconds > shift.endSecond)
        break;
    }
    if (cursor + task.durationSeconds <= shift.endSecond)
      return cursor;
  }
  return std::numeric_limits<std::int64_t>::max();
}

} // namespace

PlanValidation validateAssignmentPlan(const OptimizerSnapshot &snapshot,
                                      const AssignmentPlan &plan) {
  if (snapshot.horizonEndSecond < snapshot.capturedSecond)
    return {false, "Optimizer snapshot horizon is invalid"};

  std::unordered_set<std::uint64_t> assignedTasks;
  std::unordered_map<std::uint64_t, std::vector<OptimizationWindow>> employeePlans;
  for (const auto &assignment : plan.assignments) {
    const auto *task = taskById(snapshot, assignment.taskId);
    const auto *employee = employeeById(snapshot, assignment.employeeId);
    if (!task || !employee)
      return {false, "Plan references unknown task or employee"};
    if (!assignedTasks.insert(task->id).second)
      return {false, "Plan assigns a task more than once"};
    if (employee->absent)
      return {false, "Plan assigns an absent employee"};
    if (employee->role != task->requiredRole)
      return {false, "Plan violates task role eligibility"};
    if (assignment.startSecond < snapshot.capturedSecond ||
        assignment.startSecond < task->earliestStartSecond ||
        assignment.endSecond > snapshot.horizonEndSecond ||
        assignment.endSecond <= assignment.startSecond ||
        assignment.endSecond - assignment.startSecond < task->durationSeconds)
      return {false, "Plan assignment window is invalid"};

    const bool insideShift =
        std::any_of(employee->shiftWindows.begin(), employee->shiftWindows.end(),
                    [&](const OptimizationWindow &window) {
                      return contains(window, assignment.startSecond,
                                      assignment.endSecond);
                    });
    if (!insideShift)
      return {false, "Plan assigns work outside employee availability"};

    const OptimizationWindow assignmentWindow{assignment.startSecond,
                                               assignment.endSecond};
    if (std::any_of(employee->unavailableWindows.begin(),
                    employee->unavailableWindows.end(),
                    [&](const OptimizationWindow &window) {
                      return overlaps(window, assignmentWindow);
                    }))
      return {false, "Plan overlaps break, training, or active work"};

    auto &planned = employeePlans[employee->id];
    if (std::any_of(planned.begin(), planned.end(),
                    [&](const OptimizationWindow &window) {
                      return overlaps(window, assignmentWindow);
                    }))
      return {false, "Plan overlaps employee assignments"};
    planned.push_back(assignmentWindow);
  }
  return {true, "Plan is valid"};
}

AssignmentPlan buildDeterministicFallbackPlan(
    const OptimizerSnapshot &snapshot) {
  std::vector<OptimizerTask> tasks = snapshot.tasks;
  std::sort(tasks.begin(), tasks.end(), [](const auto &a, const auto &b) {
    if (a.critical != b.critical)
      return a.critical > b.critical;
    if (a.earliestStartSecond != b.earliestStartSecond)
      return a.earliestStartSecond < b.earliestStartSecond;
    return a.id < b.id;
  });

  std::vector<const OptimizerEmployee *> employees;
  employees.reserve(snapshot.employees.size());
  for (const auto &employee : snapshot.employees)
    employees.push_back(&employee);
  std::sort(employees.begin(), employees.end(), [](const auto *a, const auto *b) {
    return a->id < b->id;
  });

  AssignmentPlan plan;
  for (const auto &task : tasks) {
    const OptimizerEmployee *bestEmployee = nullptr;
    std::int64_t bestStart = std::numeric_limits<std::int64_t>::max();
    for (const auto *employee : employees) {
      if (employee->absent || employee->role != task.requiredRole)
        continue;
      const auto start = earliestFit(snapshot, *employee, task,
                                     plan.assignments);
      if (start < bestStart ||
          (start == bestStart && bestEmployee && employee->id < bestEmployee->id)) {
        bestStart = start;
        bestEmployee = employee;
      }
    }
    if (bestEmployee && bestStart != std::numeric_limits<std::int64_t>::max())
      plan.assignments.push_back(
          {task.id, bestEmployee->id, bestStart,
           bestStart + task.durationSeconds});
  }
  return plan;
}

} // namespace hh::game
