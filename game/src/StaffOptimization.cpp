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

bool windowLess(const OptimizationWindow &a, const OptimizationWindow &b) {
  if (a.startSecond != b.startSecond)
    return a.startSecond < b.startSecond;
  return a.endSecond < b.endSecond;
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

struct PreparedEmployee {
  const OptimizerEmployee *employee{};
  std::vector<OptimizationWindow> shiftWindows;
  std::vector<OptimizationWindow> unavailableWindows;
};

PreparedEmployee prepareEmployee(const OptimizerEmployee &employee) {
  PreparedEmployee prepared;
  prepared.employee = &employee;
  prepared.shiftWindows = employee.shiftWindows;
  prepared.unavailableWindows = employee.unavailableWindows;
  std::sort(prepared.shiftWindows.begin(), prepared.shiftWindows.end(),
            windowLess);
  std::sort(prepared.unavailableWindows.begin(),
            prepared.unavailableWindows.end(), windowLess);
  return prepared;
}

std::int64_t earliestFit(std::int64_t capturedSecond,
                         const PreparedEmployee &employee,
                         const OptimizerTask &task,
                         const std::vector<Assignment> &planned) {
  for (const auto &shift : employee.shiftWindows) {
    auto cursor = std::max({capturedSecond, task.earliestStartSecond,
                            shift.startSecond});
    if (cursor + task.durationSeconds > shift.endSecond)
      continue;
    std::size_t unavailableIndex = 0;
    std::size_t plannedIndex = 0;
    while (unavailableIndex < employee.unavailableWindows.size() ||
           plannedIndex < planned.size()) {
      OptimizationWindow blocker;
      const bool useUnavailable =
          plannedIndex == planned.size() ||
          (unavailableIndex < employee.unavailableWindows.size() &&
           windowLess(employee.unavailableWindows[unavailableIndex],
                      {planned[plannedIndex].startSecond,
                       planned[plannedIndex].endSecond}));
      if (useUnavailable) {
        blocker = employee.unavailableWindows[unavailableIndex++];
      } else {
        const auto &assignment = planned[plannedIndex++];
        blocker = {assignment.startSecond, assignment.endSecond};
      }
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

  std::vector<PreparedEmployee> preparedEmployees;
  preparedEmployees.reserve(employees.size());
  for (const auto *employee : employees)
    preparedEmployees.push_back(prepareEmployee(*employee));
  std::vector<std::vector<Assignment>> employeePlans(preparedEmployees.size());

  AssignmentPlan plan;
  for (const auto &task : tasks) {
    const OptimizerEmployee *bestEmployee = nullptr;
    std::size_t bestEmployeeIndex = 0;
    std::int64_t bestStart = std::numeric_limits<std::int64_t>::max();
    for (std::size_t employeeIndex = 0;
         employeeIndex < preparedEmployees.size(); ++employeeIndex) {
      const auto *employee = preparedEmployees[employeeIndex].employee;
      if (employee->absent || employee->role != task.requiredRole)
        continue;
      const auto start = earliestFit(
          snapshot.capturedSecond, preparedEmployees[employeeIndex], task,
          employeePlans[employeeIndex]);
      if (start < bestStart ||
          (start == bestStart && bestEmployee && employee->id < bestEmployee->id)) {
        bestStart = start;
        bestEmployee = employee;
        bestEmployeeIndex = employeeIndex;
      }
    }
    if (bestEmployee && bestStart != std::numeric_limits<std::int64_t>::max()) {
      plan.assignments.push_back(
          {task.id, bestEmployee->id, bestStart,
           bestStart + task.durationSeconds});
      const Assignment assignment = plan.assignments.back();
      auto &employeePlan = employeePlans[bestEmployeeIndex];
      const auto insertion = std::lower_bound(
          employeePlan.begin(), employeePlan.end(), assignment,
          [](const Assignment &plannedAssignment,
             const Assignment &candidate) {
            if (plannedAssignment.startSecond != candidate.startSecond)
              return plannedAssignment.startSecond < candidate.startSecond;
            return plannedAssignment.endSecond < candidate.endSecond;
          });
      employeePlan.insert(insertion, assignment);
    }
  }
  return plan;
}

} // namespace hh::game
