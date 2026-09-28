#include "hh/game/Workforce.h"

#include <array>
#include <limits>

namespace hh::game {
namespace {

std::uint64_t mix(std::uint64_t x) {
  x += 0x9e3779b97f4a7c15ULL;
  x = (x ^ (x >> 30U)) * 0xbf58476d1ce4e5b9ULL;
  x = (x ^ (x >> 27U)) * 0x94d049bb133111ebULL;
  return x ^ (x >> 31U);
}

std::uint64_t sample(std::uint64_t seed, int day, std::uint64_t index,
                     std::uint64_t stream) {
  return mix(seed ^ (static_cast<std::uint64_t>(day) << 32U) ^
             (index * 0x9e3779b97f4a7c15ULL) ^ stream);
}

} // namespace

std::vector<Applicant> Workforce::applicantPool(std::uint64_t campaignSeed,
                                                int simDay) {
  static constexpr std::array<const char *, 12> firstNames{
      "Avery", "Bailey", "Cameron", "Devin", "Emery", "Finley",
      "Harper", "Jordan", "Morgan", "Parker", "Quinn", "Riley"};
  static constexpr std::array<const char *, 12> lastNames{
      "Adler", "Bennett", "Cruz", "Dawson", "Ellis", "Foster",
      "Garcia", "Hayes", "Ito", "Khan", "Lopez", "Meyer"};

  if (simDay < 0)
    return {};

  std::vector<Applicant> out;
  out.reserve(6);
  for (std::uint64_t index = 0; index < 6; ++index) {
    const auto roleValue = sample(campaignSeed, simDay, index, 1) % 3;
    const auto role = static_cast<StaffRole>(roleValue);
    const int baseWage = role == StaffRole::Receptionist ? 2000
                         : role == StaffRole::Maintenance ? 2500
                                                          : 1800;

    Applicant applicant;
    applicant.id = mix(campaignSeed ^
                       (static_cast<std::uint64_t>(simDay) << 24U) ^
                       (index + 1U));
    if (applicant.id == 0)
      applicant.id = std::numeric_limits<ApplicantId>::max() - index;
    applicant.name = std::string(firstNames[sample(campaignSeed, simDay, index, 2) %
                                                firstNames.size()]) +
                     " " +
                     lastNames[sample(campaignSeed, simDay, index, 3) %
                               lastNames.size()];
    applicant.role = role;
    applicant.wageExpectationCents =
        baseWage + static_cast<int>(sample(campaignSeed, simDay, index, 4) % 701);
    applicant.skill =
        45 + static_cast<int>(sample(campaignSeed, simDay, index, 5) % 46);
    applicant.reliability =
        70 + static_cast<int>(sample(campaignSeed, simDay, index, 6) % 31);
    applicant.shiftStartHour =
        static_cast<int>(sample(campaignSeed, simDay, index, 7) % 16);
    const int shiftHours =
        6 + static_cast<int>(sample(campaignSeed, simDay, index, 8) % 4);
    applicant.shiftEndHour = (applicant.shiftStartHour + shiftHours) % 24;
    out.push_back(std::move(applicant));
  }
  return out;
}

bool Workforce::absentForShift(std::uint64_t campaignSeed,
                               std::uint64_t employeeId,
                               std::int64_t shiftKey,
                               double reliabilityPercent) {
  if (reliabilityPercent >= 100.0)
    return false;
  if (reliabilityPercent <= 0.0)
    return true;
  const auto key = campaignSeed ^ mix(employeeId) ^
                   mix(static_cast<std::uint64_t>(shiftKey));
  const double roll = static_cast<double>(mix(key) % 10000ULL) / 100.0;
  return roll >= reliabilityPercent;
}

} // namespace hh::game
