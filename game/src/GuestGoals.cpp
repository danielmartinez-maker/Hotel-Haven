#include "hh/game/GuestGoals.h"
#include <algorithm>
#include <limits>

namespace hh::game {
namespace {
constexpr std::int64_t factorScale = 10000;
constexpr int maxFactor = 15000;

std::int64_t multiplyFactor(std::int64_t value, int factor) noexcept {
  const auto bounded = std::clamp(factor, 0, maxFactor);
  if (value <= 0 || bounded == 0)
    return 0;
  if (value > std::numeric_limits<std::int64_t>::max() / bounded)
    return std::numeric_limits<std::int64_t>::max();
  return value * bounded / factorScale;
}

int distanceUtility(const GoalOpportunity &opportunity) noexcept {
  const auto travel = std::max(0, opportunity.expectedTravelSeconds);
  const auto wait = std::max(0, opportunity.expectedWaitSeconds);
  const std::int64_t seconds = static_cast<std::int64_t>(travel) + wait;
  return static_cast<int>((factorScale * 600) / (600 + seconds));
}
} // namespace

std::int64_t scoreGuestGoal(const GoalOpportunity &opportunity) noexcept {
  const int need = std::clamp(opportunity.needScore, 0, 100);
  const std::int64_t deficit = 100 - need;
  std::int64_t utility = deficit * deficit * 2;
  utility = multiplyFactor(utility, opportunity.preference);
  utility = multiplyFactor(utility, opportunity.availability);
  utility = multiplyFactor(utility, opportunity.timeCompatibility);
  utility = multiplyFactor(utility, opportunity.budgetCompatibility);
  utility = multiplyFactor(
      utility, need < 15 ? static_cast<int>(factorScale)
                         : opportunity.groupCompatibility);
  utility = multiplyFactor(utility, distanceUtility(opportunity));
  utility = multiplyFactor(utility, opportunity.moodModifier);
  return utility;
}

GoalSelection chooseGuestGoal(EntityId guestId,
                              const GuestOpportunitySnapshot &snapshot) noexcept {
  (void)guestId;
  const bool hasMandatory = std::any_of(
      snapshot.opportunities.begin(), snapshot.opportunities.end(),
      [](const GoalOpportunity &opportunity) { return opportunity.mandatory; });

  GoalSelection best;
  for (const auto &opportunity : snapshot.opportunities) {
    if (hasMandatory && !opportunity.mandatory)
      continue;
    const auto utility = scoreGuestGoal(opportunity);
    if (!best.valid || utility > best.utility ||
        (utility == best.utility && opportunity.stableGoalId < best.stableGoalId)) {
      best.valid = true;
      best.stableGoalId = opportunity.stableGoalId;
      best.goal = opportunity.goal;
      best.utility = utility;
    }
  }
  return best;
}

bool acceptsGroupProposal(std::int64_t bestIndividualUtility,
                          std::int64_t proposedGroupUtility) noexcept {
  if (bestIndividualUtility <= 0)
    return proposedGroupUtility >= bestIndividualUtility;
  if (proposedGroupUtility < 0)
    return false;
  const auto quotient = bestIndividualUtility / 10;
  const auto remainder = bestIndividualUtility % 10;
  const auto threshold = quotient * 7 + (remainder * 7 + 9) / 10;
  return proposedGroupUtility >= threshold;
}

} // namespace hh::game
