#include "StressHarness.h"
#include "hh/game/Departments.h"
#include "hh/game/Simulation.h"
#include "hh/game/StaffOptimization.h"
#include "hh/game/Workforce.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {
using namespace hh::game;

struct ScaleBudget {
  int employees;
  std::size_t assignmentOperations;
  int replans;
};

ScaleBudget budget(hh::stress::Scale scale) {
  switch (scale) {
  case hh::stress::Scale::Pr:
    return {2'000, 20'000, 2};
  case hh::stress::Scale::Extended:
    return {5'000, 200'000, 4};
  case hh::stress::Scale::Exhaustive:
    return {10'000, 1'000'000, 8};
  }
  std::abort();
}

PersonKind personKindFor(int index) {
  switch (index % 3) {
  case 0:
    return PersonKind::Receptionist;
  case 1:
    return PersonKind::Housekeeper;
  default:
    return PersonKind::Maintenance;
  }
}

StaffRole staffRoleFor(int index) {
  switch (index % 3) {
  case 0:
    return StaffRole::Receptionist;
  case 1:
    return StaffRole::Housekeeper;
  default:
    return StaffRole::Maintenance;
  }
}

bool liveEmployee(const SimulationView &view, EntityId id) {
  return std::any_of(view.people.begin(), view.people.end(),
                     [id](const PersonView &person) {
                       return person.id == id && person.kind != PersonKind::Guest;
                     });
}

void assertSimulationAssignments(const SimulationView &view,
                                 hh::stress::RunContext &ctx,
                                 std::uint64_t checkpoint) {
  std::unordered_set<EntityId> active;
  for (const auto &task : view.tasks) {
    if (task.status != TaskStatus::Traveling && task.status != TaskStatus::Working)
      continue;
    if (task.employeeId == 0 || !liveEmployee(view, task.employeeId))
      ctx.fail("active task references missing employee", checkpoint);
    if (!active.insert(task.employeeId).second)
      ctx.fail("employee has overlapping active simulation tasks", checkpoint);
    const auto employee = std::find_if(
        view.people.begin(), view.people.end(),
        [&](const PersonView &person) { return person.id == task.employeeId; });
    if (employee == view.people.end() || employee->absent)
      ctx.fail("absent employee has active task", checkpoint);
  }
}

Simulation buildRoster(const ScaleBudget &limits, const hh::stress::Config &config,
                       hh::stress::RunContext &ctx) {
  Simulation sim(config.seed, 16, 12, 1);
  if (!sim.buildTile({0, 0, 0}, TileKind::Entrance).ok)
    ctx.fail("roster entrance build failed", 0);
  for (int index = 0; index < limits.employees; ++index) {
    const auto hire = sim.hireStaff({"Stress " + std::to_string(index),
                                     personKindFor(index), 0, 0,
                                     18.0 + static_cast<double>(index % 7)});
    if (!hire.ok)
      ctx.fail("roster growth hire failed", static_cast<std::uint64_t>(index));
    if ((index & 255) == 0)
      ctx.trace.push("hired=" + std::to_string(index + 1));
  }
  return sim;
}

void rosterAndShiftStress(const ScaleBudget &limits,
                          const hh::stress::Config &config) {
  hh::stress::RunContext ctx{"workforce", config};
  ctx.phase = "roster_growth";
  auto sim = buildRoster(limits, config, ctx);
  auto view = sim.view();
  int staffCount = 0;
  for (const auto &person : view.people)
    staffCount += person.kind != PersonKind::Guest;
  if (staffCount != limits.employees)
    ctx.fail("roster cardinality mismatch", staffCount);

  ctx.phase = "shift_churn";
  std::vector<EntityId> staff;
  staff.reserve(limits.employees);
  for (const auto &person : view.people)
    if (person.kind != PersonKind::Guest)
      staff.push_back(person.id);
  std::sort(staff.begin(), staff.end());
  for (int pass = 0; pass < 3; ++pass) {
    for (std::size_t index = 0; index < staff.size(); ++index) {
      const int start = static_cast<int>((index + pass * 3) % 24);
      const int end = (start + 8) % 24;
      if (!sim.setStaffShift(staff[index], start, end).ok)
        ctx.fail("valid shift churn rejected", index);
    }
    assertSimulationAssignments(sim.view(), ctx, pass);
  }

  const auto snapshot = sim.buildOptimizerSnapshot();
  if (snapshot.employees.size() != static_cast<std::size_t>(limits.employees))
    ctx.fail("optimizer snapshot omitted workforce", snapshot.employees.size());

  const auto departments = sim.departments();
  if (departments.size() != allDepartments().size())
    ctx.fail("department cardinality changed", departments.size());
  for (const auto &department : departments) {
    for (const auto report : department.directReports)
      if (!liveEmployee(sim.view(), report))
        ctx.fail("department references missing direct report", report);
    if (department.managerId != 0 && !liveEmployee(sim.view(), department.managerId))
      ctx.fail("department references missing manager", department.managerId);
  }
}

void absenceStress(const ScaleBudget &limits, const hh::stress::Config &config) {
  hh::stress::RunContext ctx{"workforce", config};
  ctx.phase = "absence_spike";
  const auto checks = std::max<std::size_t>(20'000, limits.assignmentOperations);
  std::size_t absences = 0;
  for (std::size_t i = 0; i < checks; ++i) {
    const auto employee = static_cast<std::uint64_t>(i % limits.employees + 1);
    const auto shift = static_cast<std::int64_t>(i / limits.employees);
    const auto first = Workforce::absentForShift(config.seed, employee, shift, 50.0);
    const auto second = Workforce::absentForShift(config.seed, employee, shift, 50.0);
    if (first != second)
      ctx.fail("absence decision was not deterministic", i);
    absences += first ? 1U : 0U;
  }
  if (absences == 0 || absences == checks)
    ctx.fail("absence stress did not exercise mixed outcomes", checks);
}

OptimizerSnapshot makeOptimizerSnapshot(const ScaleBudget &limits,
                                        const hh::stress::Config &config) {
  OptimizerSnapshot snapshot;
  snapshot.capturedSecond = 0;
  snapshot.horizonEndSecond = 86'400;
  snapshot.employees.reserve(limits.employees);
  for (int index = 0; index < limits.employees; ++index) {
    OptimizerEmployee employee;
    employee.id = static_cast<std::uint64_t>(index + 1);
    employee.role = staffRoleFor(index);
    employee.absent = (index % 31) == 0;
    employee.availableNow = !employee.absent;
    employee.shiftWindows.push_back({0, 86'400});
    if ((index % 17) == 0)
      employee.unavailableWindows.push_back({18'000, 18'900});
    snapshot.employees.push_back(std::move(employee));
  }

  snapshot.tasks.reserve(limits.assignmentOperations);
  hh::stress::Rng rng{config.seed ^ 0x9E3779B97F4A7C15ULL};
  for (std::size_t index = 0; index < limits.assignmentOperations; ++index) {
    OptimizerTask task;
    task.id = static_cast<std::uint64_t>(index + 1);
    task.requiredRole = staffRoleFor(static_cast<int>(index));
    task.critical = (index % 11) == 0;
    task.earliestStartSecond =
        static_cast<std::int64_t>(rng.bounded(20 * 3600));
    task.durationSeconds = 30 + static_cast<int>(rng.bounded(271));
    snapshot.tasks.push_back(task);
  }
  return snapshot;
}

void assertPlanReferences(const OptimizerSnapshot &snapshot,
                          const AssignmentPlan &plan,
                          hh::stress::RunContext &ctx,
                          std::uint64_t checkpoint) {
  std::unordered_map<std::uint64_t, const OptimizerEmployee *> employees;
  std::unordered_set<std::uint64_t> tasks;
  for (const auto &employee : snapshot.employees)
    employees.emplace(employee.id, &employee);
  for (const auto &task : snapshot.tasks)
    tasks.insert(task.id);

  std::unordered_map<std::uint64_t, std::vector<Assignment>> byEmployee;
  for (const auto &assignment : plan.assignments) {
    const auto employee = employees.find(assignment.employeeId);
    if (employee == employees.end())
      ctx.fail("plan references missing employee", checkpoint);
    if (employee->second->absent)
      ctx.fail("plan assigns absent employee", checkpoint);
    if (!tasks.contains(assignment.taskId))
      ctx.fail("plan references missing task", checkpoint);
    if (assignment.endSecond < assignment.startSecond)
      ctx.fail("assignment ends before it starts", checkpoint);
    byEmployee[assignment.employeeId].push_back(assignment);
  }

  for (auto &[employee, assignments] : byEmployee) {
    (void)employee;
    std::sort(assignments.begin(), assignments.end(),
              [](const Assignment &a, const Assignment &b) {
                if (a.startSecond != b.startSecond)
                  return a.startSecond < b.startSecond;
                return a.taskId < b.taskId;
              });
    for (std::size_t i = 1; i < assignments.size(); ++i)
      if (assignments[i].startSecond < assignments[i - 1].endSecond)
        ctx.fail("employee has overlapping exclusive assignments", checkpoint);
  }
}

void optimizerStress(const ScaleBudget &limits,
                     const hh::stress::Config &config) {
  hh::stress::RunContext ctx{"workforce", config};
  ctx.phase = "task_saturation";
  const auto snapshot = makeOptimizerSnapshot(limits, config);
  if (snapshot.tasks.size() != limits.assignmentOperations)
    ctx.fail("optimizer task fixture lost work", snapshot.tasks.size());

  AssignmentPlan baseline;
  for (int replan = 0; replan < limits.replans; ++replan) {
    ctx.phase = replan == 0 ? "optimizer_replan" : "fallback_replay";
    const auto plan = buildDeterministicFallbackPlan(snapshot);
    const auto validation = validateAssignmentPlan(snapshot, plan);
    if (!validation.ok)
      ctx.fail("native fallback generated invalid assignment plan", replan);
    assertPlanReferences(snapshot, plan, ctx, replan);
    if (replan == 0)
      baseline = plan;
    else if (!(plan == baseline))
      ctx.fail("fallback optimizer replay diverged", replan);
  }
  if (baseline.assignments.empty())
    ctx.fail("fallback optimizer assigned no work", 0);
}

void validateScenario(std::string_view scenario) {
  if (scenario.empty() || scenario == "roster_growth" ||
      scenario == "shift_churn" || scenario == "absence_spike" ||
      scenario == "task_saturation" || scenario == "optimizer_replan" ||
      scenario == "fallback_replay")
    return;
  throw std::invalid_argument("unknown workforce stress scenario");
}
} // namespace

int main() {
  try {
    const auto config = hh::stress::configFromEnvironment(0x5EED5EED5EED5EEDULL);
    validateScenario(config.scenario);
    const auto limits = budget(config.scale);

    if (config.scenario.empty() || config.scenario == "roster_growth" ||
        config.scenario == "shift_churn")
      rosterAndShiftStress(limits, config);
    if (config.scenario.empty() || config.scenario == "absence_spike")
      absenceStress(limits, config);
    if (config.scenario.empty() || config.scenario == "task_saturation" ||
        config.scenario == "optimizer_replan" ||
        config.scenario == "fallback_replay")
      optimizerStress(limits, config);

    std::cout << "StressWorkforce PASS employees=" << limits.employees
              << " assignment_ops=" << limits.assignmentOperations << '\n';
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
