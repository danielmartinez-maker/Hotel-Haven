#pragma once

#include "hh/game/Workforce.h"

#include <cstdint>
#include <string>
#include <vector>

namespace hh::game {

using SimDay = int;

enum class DepartmentId : std::uint8_t {
  FrontOffice,
  Housekeeping,
  Engineering
};

struct ManagerAssignment {
  DepartmentId department{DepartmentId::FrontOffice};
  std::uint64_t managerId{};
  bool operator==(const ManagerAssignment &) const = default;
};

struct ForecastBucket {
  int startMinute{};
  int endMinute{};
  int requiredMinutes{};
  int scheduledMinutes{};
  int uncoveredMinutes{};
  bool operator==(const ForecastBucket &) const = default;
};

struct DepartmentForecast {
  DepartmentId department{DepartmentId::FrontOffice};
  SimDay day{};
  int requiredMinutes{};
  int scheduledMinutes{};
  int uncoveredMinutes{};
  std::vector<ForecastBucket> buckets;
  bool operator==(const DepartmentForecast &) const = default;
};

struct DepartmentView {
  DepartmentId id{DepartmentId::FrontOffice};
  std::string name;
  StaffRole role{StaffRole::Receptionist};
  std::uint64_t managerId{};
  std::vector<std::uint64_t> directReports;
  bool operator==(const DepartmentView &) const = default;
};

[[nodiscard]] StaffRole departmentRole(DepartmentId department);
[[nodiscard]] const char *departmentName(DepartmentId department);
[[nodiscard]] const std::vector<DepartmentId> &allDepartments();

} // namespace hh::game
