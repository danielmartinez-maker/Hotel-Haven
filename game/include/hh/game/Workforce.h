#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace hh::game {

using ApplicantId = std::uint64_t;

enum class StaffRole : std::uint8_t {
  Receptionist,
  Housekeeper,
  Maintenance
};

struct Applicant {
  ApplicantId id{};
  std::string name;
  StaffRole role{StaffRole::Housekeeper};
  std::int64_t wageExpectationCents{};
  int skill{};
  int reliability{};
  int shiftStartHour{};
  int shiftEndHour{};

  bool operator==(const Applicant &) const = default;
};

struct EmployeeContract {
  ApplicantId sourceApplicantId{};
  StaffRole role{StaffRole::Housekeeper};
  std::int64_t hourlyWageCents{};
  std::int64_t onboardingCostCents{};
  int shiftStartHour{};
  int shiftEndHour{};

  bool operator==(const EmployeeContract &) const = default;
};

struct HireResult {
  bool ok{};
  std::string message;
  std::uint64_t employeeId{};
  ApplicantId applicantId{};
  explicit operator bool() const noexcept { return ok; }
};

class Workforce {
public:
  [[nodiscard]] static std::vector<Applicant>
  applicantPool(std::uint64_t campaignSeed, int simDay);
  [[nodiscard]] static bool absentForShift(std::uint64_t campaignSeed,
                                           std::uint64_t employeeId,
                                           std::int64_t shiftKey,
                                           double reliabilityPercent);
};

} // namespace hh::game
