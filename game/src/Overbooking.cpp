#include "hh/game/Overbooking.h"

#include <algorithm>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace hh::game {

OverbookingResult OverbookingSystem::setPolicy(const OverbookingPolicy &policy) {
  const bool invalidAllowance =
      policy.allowanceMode == OverbookingAllowanceMode::Rooms
          ? policy.allowance < 0
          : (policy.allowanceBasisPoints < 0 || policy.allowanceBasisPoints > 10000);
  if (policy.roomCategory.empty() || invalidAllowance ||
      policy.relocationCompensationCents < 0 || policy.startDay < 0 ||
      policy.endDay < policy.startDay)
    return {false, "INVALID_OVERBOOKING_POLICY"};
  policies_[policy.roomCategory] = policy;
  return {true, "OK"};
}

int OverbookingSystem::allowance(std::string_view category) const {
  const auto it = policies_.find(std::string(category));
  if (it == policies_.end() ||
      it->second.allowanceMode != OverbookingAllowanceMode::Rooms)
    return 0;
  return it->second.allowance;
}

int OverbookingSystem::allowance(std::string_view category, int day) const {
  const auto it = policies_.find(std::string(category));
  if (it == policies_.end() || day < it->second.startDay || day > it->second.endDay ||
      it->second.allowanceMode != OverbookingAllowanceMode::Rooms)
    return 0;
  return it->second.allowance;
}

int OverbookingSystem::allowance(std::string_view category, int day,
                                 int physicalRooms) const {
  if (physicalRooms < 0)
    return 0;
  const auto it = policies_.find(std::string(category));
  if (it == policies_.end() || day < it->second.startDay || day > it->second.endDay)
    return 0;
  const auto &policy = it->second;
  if (policy.allowanceMode == OverbookingAllowanceMode::Rooms)
    return policy.allowance;
  const auto resolved =
      static_cast<std::int64_t>(physicalRooms) * policy.allowanceBasisPoints / 10000;
  return static_cast<int>(std::min<std::int64_t>(
      resolved, static_cast<std::int64_t>(std::numeric_limits<int>::max())));
}

RecoveryDecision OverbookingSystem::chooseRecovery(const RecoveryContext &context) const {
  if (context.categoryEquivalentAvailable)
    return {RecoveryAction::CategoryEquivalentRoom, 0, "CATEGORY_EQUIVALENT_ROOM",
            false, ""};
  if (context.freeUpgradeAvailable)
    return {RecoveryAction::FreeUpgrade, 0, "FREE_UPGRADE", false, ""};
  if (context.paidUpgradeAvailable && context.guestAcceptsPaidUpgrade)
    return {RecoveryAction::PaidUpgrade, 0, "PAID_UPGRADE_ACCEPTED", false, ""};
  if (context.accelerationFeasible)
    return {RecoveryAction::AccelerateRoomRecovery, 0, "ACCELERATE_OWNING_SYSTEM",
            false, ""};
  if (context.competitorRelocationAvailable) {
    const auto it = policies_.find(context.roomCategory);
    const auto compensation =
        it == policies_.end() ? 0 : it->second.relocationCompensationCents;
    return {RecoveryAction::CompetitorRelocation, compensation,
            "COMPETITOR_RELOCATION_AND_COMPENSATION", false, ""};
  }
  return {RecoveryAction::Unresolved, 0, "NO_FEASIBLE_RECOVERY", true,
          "OVERBOOKING_UNRESOLVED_SEVERE"};
}

const std::map<std::string, OverbookingPolicy> &OverbookingSystem::policies() const noexcept {
  return policies_;
}

std::string OverbookingSystem::save() const {
  std::ostringstream out;
  out << "HHOVER 3 " << policies_.size();
  for (const auto &[category, policy] : policies_)
    out << ' ' << std::quoted(category) << ' ' << policy.allowance << ' '
        << policy.relocationCompensationCents << ' ' << policy.startDay << ' '
        << policy.endDay << ' ' << static_cast<int>(policy.allowanceMode) << ' '
        << policy.allowanceBasisPoints;
  return out.str();
}

OverbookingSystem OverbookingSystem::load(std::string_view data) {
  std::istringstream in{std::string(data)};
  std::string magic;
  int version{};
  std::size_t count{};
  in >> magic >> version >> count;
  if (!in || magic != "HHOVER" ||
      (version != 1 && version != 2 && version != 3) || count > 10000)
    throw std::invalid_argument("invalid overbooking save");
  OverbookingSystem result;
  for (std::size_t i = 0; i < count; ++i) {
    OverbookingPolicy policy;
    in >> std::quoted(policy.roomCategory) >> policy.allowance >>
        policy.relocationCompensationCents;
    if (version >= 2)
      in >> policy.startDay >> policy.endDay;
    if (version >= 3) {
      int mode{};
      in >> mode >> policy.allowanceBasisPoints;
      if (!in || mode < static_cast<int>(OverbookingAllowanceMode::Rooms) ||
          mode > static_cast<int>(OverbookingAllowanceMode::Percentage))
        throw std::invalid_argument("invalid saved overbooking mode");
      policy.allowanceMode = static_cast<OverbookingAllowanceMode>(mode);
    }
    if (!in || !result.setPolicy(policy).ok)
      throw std::invalid_argument("invalid saved overbooking policy");
  }
  in >> std::ws;
  if (!in.eof())
    throw std::invalid_argument("unexpected overbooking trailing data");
  return result;
}

} // namespace hh::game
