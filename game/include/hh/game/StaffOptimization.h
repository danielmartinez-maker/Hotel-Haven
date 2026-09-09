#pragma once

#include "hh/game/Workforce.h"

#include <cstdint>
#include <string>
#include <vector>

namespace hh::game {

struct OptimizationWindow {
  std::int64_t startSecond{};
  std::int64_t endSecond{};
  bool operator==(const OptimizationWindow &) const = default;
};

struct OptimizerEmployee {
  std::uint64_t id{};
  StaffRole role{StaffRole::Housekeeper};
  bool absent{};
  bool availableNow{};
  std::vector<OptimizationWindow> shiftWindows;
  std::vector<OptimizationWindow> unavailableWindows;
  bool operator==(const OptimizerEmployee &) const = default;
};

struct OptimizerTask {
  std::uint64_t id{};
  StaffRole requiredRole{StaffRole::Housekeeper};
  bool critical{};
  std::int64_t earliestStartSecond{};
  int durationSeconds{};
  bool operator==(const OptimizerTask &) const = default;
};

struct OptimizerSnapshot {
  std::int64_t capturedSecond{};
  std::int64_t horizonEndSecond{};
  std::vector<OptimizerEmployee> employees;
  std::vector<OptimizerTask> tasks;
  bool operator==(const OptimizerSnapshot &) const = default;
};

struct Assignment {
  std::uint64_t taskId{};
  std::uint64_t employeeId{};
  std::int64_t startSecond{};
  std::int64_t endSecond{};
  bool operator==(const Assignment &) const = default;
};

struct AssignmentPlan {
  std::vector<Assignment> assignments;
  bool operator==(const AssignmentPlan &) const = default;
};

struct PlanValidation {
  bool ok{};
  std::string message;
  explicit operator bool() const noexcept { return ok; }
  bool operator==(const PlanValidation &) const = default;
};

[[nodiscard]] PlanValidation
validateAssignmentPlan(const OptimizerSnapshot &snapshot,
                       const AssignmentPlan &plan);
[[nodiscard]] AssignmentPlan
buildDeterministicFallbackPlan(const OptimizerSnapshot &snapshot);

} // namespace hh::game
