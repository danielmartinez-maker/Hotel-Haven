#include "hh/game/GuestGoals.h"
#include "hh/game/GuestPsychology.h"
#include <algorithm>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

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

int preferenceForGoal(const GuestPreferenceState &preferences,
                      GuestGoalClass goal) noexcept {
  switch (goal) {
  case GuestGoalClass::Sleep:
    return preferences.quietRoom;
  case GuestGoalClass::Eat:
    return preferences.breakfast;
  case GuestGoalClass::Drink:
    return preferences.social;
  case GuestGoalClass::Work:
    return (preferences.wifi + preferences.desk) / 2;
  case GuestGoalClass::Exercise:
    return preferences.fitness;
  case GuestGoalClass::Swim:
    return preferences.pool;
  case GuestGoalClass::Socialize:
  case GuestGoalClass::AttendEvent:
    return preferences.social;
  case GuestGoalClass::Relax:
    return (preferences.spa + preferences.roomQuality) / 2;
  case GuestGoalClass::RequestService:
    return preferences.roomQuality;
  default:
    return static_cast<int>(factorScale);
  }
}

int combinePreference(int opportunityPreference,
                      int guestPreference) noexcept {
  const auto base = std::clamp(opportunityPreference, 0, maxFactor);
  const auto guest = std::clamp(guestPreference, 0, 10000);
  const auto combined = static_cast<std::int64_t>(base) * guest / factorScale;
  return static_cast<int>(std::clamp<std::int64_t>(combined, 0, maxFactor));
}

bool validGoalValue(GuestGoalClass goal) noexcept {
  const auto value = static_cast<int>(goal);
  return value >= static_cast<int>(GuestGoalClass::ReachHotel) &&
         value <= static_cast<int>(GuestGoalClass::LeaveHotel);
}

int routineMultiplier(const GuestProfileView &profile, GuestGoalClass goal,
                      int hour) noexcept {
  const int normalizedHour = ((hour % 24) + 24) % 24;
  const bool morning = normalizedHour >= 5 && normalizedHour < 11;
  const bool daytime = normalizedHour >= 8 && normalizedHour < 18;
  const bool evening = normalizedHour >= 18 && normalizedHour < 24;
  int multiplier = 10000;

  switch (profile.archetype) {
  case GuestArchetype::BusinessTraveler:
    if (goal == GuestGoalClass::Work && daytime) multiplier = 13500;
    else if (goal == GuestGoalClass::Relax && evening) multiplier = 11500;
    else if (goal == GuestGoalClass::Swim && daytime) multiplier = 7500;
    break;
  case GuestArchetype::ExecutiveBusiness:
    if (goal == GuestGoalClass::Work && daytime) multiplier = 14000;
    else if (goal == GuestGoalClass::RequestService) multiplier = 12000;
    else if (goal == GuestGoalClass::Socialize && evening) multiplier = 11000;
    break;
  case GuestArchetype::DigitalNomad:
    if (goal == GuestGoalClass::Work && daytime) multiplier = 13000;
    else if (goal == GuestGoalClass::Socialize && evening) multiplier = 12000;
    else if (goal == GuestGoalClass::Relax && daytime) multiplier = 11000;
    break;
  case GuestArchetype::BleisureTraveler:
    if (goal == GuestGoalClass::Work && daytime) multiplier = 12000;
    else if ((goal == GuestGoalClass::Relax ||
              goal == GuestGoalClass::Socialize) && evening)
      multiplier = 13000;
    break;
  case GuestArchetype::ExtendedStayGuest:
    if (goal == GuestGoalClass::Work && daytime) multiplier = 11500;
    else if (goal == GuestGoalClass::Relax) multiplier = 11000;
    break;
  case GuestArchetype::AirlineCrew:
    if (goal == GuestGoalClass::Sleep) multiplier = 14500;
    else if (goal == GuestGoalClass::Relax) multiplier = 12000;
    else if (goal == GuestGoalClass::Socialize) multiplier = 6500;
    break;
  case GuestArchetype::WeddingGuest:
    if (goal == GuestGoalClass::AttendEvent && evening) multiplier = 15000;
    else if (goal == GuestGoalClass::Socialize && evening) multiplier = 14500;
    else if (goal == GuestGoalClass::Sleep && evening) multiplier = 7000;
    break;
  case GuestArchetype::StaycationGuest:
    if ((goal == GuestGoalClass::Relax || goal == GuestGoalClass::Swim) &&
        daytime)
      multiplier = 14500;
    else if (goal == GuestGoalClass::Work) multiplier = 5000;
    break;
  case GuestArchetype::SportsTeamTraveler:
    if (goal == GuestGoalClass::Exercise && morning) multiplier = 14500;
    else if (goal == GuestGoalClass::Eat && morning) multiplier = 12500;
    else if (goal == GuestGoalClass::Socialize && evening) multiplier = 11500;
    break;
  case GuestArchetype::WellnessTraveler:
    if (goal == GuestGoalClass::Exercise && morning) multiplier = 14000;
    else if (goal == GuestGoalClass::Relax && daytime) multiplier = 14000;
    else if (goal == GuestGoalClass::Drink) multiplier = 7000;
    break;
  case GuestArchetype::FamilyLeisure:
    if (goal == GuestGoalClass::Swim && daytime) multiplier = 13000;
    else if (goal == GuestGoalClass::Eat && morning) multiplier = 12000;
    break;
  case GuestArchetype::CoupleLeisure:
    if (goal == GuestGoalClass::Relax && evening) multiplier = 12500;
    else if (goal == GuestGoalClass::Socialize && evening) multiplier = 11500;
    break;
  case GuestArchetype::ConferenceDelegate:
    if (goal == GuestGoalClass::AttendEvent && daytime) multiplier = 15000;
    else if (goal == GuestGoalClass::Socialize && evening) multiplier = 12000;
    break;
  case GuestArchetype::LuxuryLeisure:
  case GuestArchetype::VipCelebrity:
    if (goal == GuestGoalClass::Relax && daytime) multiplier = 13000;
    else if (goal == GuestGoalClass::RequestService) multiplier = 12500;
    break;
  default:
    break;
  }

  if ((profile.traitFlags & guestTraitFlag(GuestTrait::EarlyRiser)) != 0) {
    if (goal == GuestGoalClass::Exercise && morning)
      multiplier = std::min(maxFactor, multiplier + 1500);
    if (goal == GuestGoalClass::Sleep && morning)
      multiplier = std::max(0, multiplier - 2000);
  }
  if ((profile.traitFlags & guestTraitFlag(GuestTrait::NightOwl)) != 0) {
    if (goal == GuestGoalClass::Socialize && evening)
      multiplier = std::min(maxFactor, multiplier + 1500);
    if (goal == GuestGoalClass::Sleep && evening)
      multiplier = std::max(0, multiplier - 1500);
  }
  return std::clamp(multiplier, 0, maxFactor);
}

int applyRoutineMultiplier(int compatibility, int multiplier) noexcept {
  return static_cast<int>(std::clamp<std::int64_t>(
      static_cast<std::int64_t>(std::clamp(compatibility, 0, maxFactor)) *
          std::clamp(multiplier, 0, maxFactor) / factorScale,
      0, maxFactor));
}

const GoalOpportunity *selectedOpportunity(
    const GoalSelection &selection,
    const GuestOpportunitySnapshot &snapshot) noexcept {
  const auto found = std::find_if(
      snapshot.opportunities.begin(), snapshot.opportunities.end(),
      [&](const GoalOpportunity &opportunity) {
        return opportunity.stableGoalId == selection.stableGoalId;
      });
  return found == snapshot.opportunities.end() ? nullptr : &*found;
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

GuestOpportunitySnapshot
applyGuestPreferences(const GuestPreferenceState &preferences,
                      const GuestOpportunitySnapshot &snapshot) noexcept {
  GuestOpportunitySnapshot personalized = snapshot;
  for (auto &opportunity : personalized.opportunities) {
    if (opportunity.mandatory)
      continue;
    opportunity.preference = combinePreference(
        opportunity.preference, preferenceForGoal(preferences, opportunity.goal));
  }
  return personalized;
}

GuestOpportunitySnapshot
applyGuestArchetypeRoutine(const GuestProfileView &profile, int hour,
                           const GuestOpportunitySnapshot &snapshot) noexcept {
  GuestOpportunitySnapshot scheduled = snapshot;
  for (auto &opportunity : scheduled.opportunities) {
    if (opportunity.mandatory)
      continue;
    opportunity.timeCompatibility = applyRoutineMultiplier(
        opportunity.timeCompatibility,
        routineMultiplier(profile, opportunity.goal, hour));
  }
  return scheduled;
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

GoalSelection
chooseGuestGroupMemberGoal(EntityId guestId, const GuestGroup &group,
                           const GuestOpportunitySnapshot &snapshot) noexcept {
  const auto individual = chooseGuestGoal(guestId, snapshot);
  if (!individual.valid || guestId == group.leader || group.sharedItinerary.empty() ||
      std::find(group.members.begin(), group.members.end(), guestId) ==
          group.members.end())
    return individual;

  const auto *individualOpportunity = selectedOpportunity(individual, snapshot);
  if (!individualOpportunity || individualOpportunity->mandatory ||
      individualOpportunity->needScore < 15)
    return individual;

  const auto proposedGoal = group.sharedItinerary.front();
  GoalSelection proposed;
  for (const auto &opportunity : snapshot.opportunities) {
    if (opportunity.mandatory || opportunity.goal != proposedGoal)
      continue;
    const auto utility = scoreGuestGoal(opportunity);
    if (!proposed.valid || utility > proposed.utility ||
        (utility == proposed.utility &&
         opportunity.stableGoalId < proposed.stableGoalId)) {
      proposed.valid = true;
      proposed.stableGoalId = opportunity.stableGoalId;
      proposed.goal = opportunity.goal;
      proposed.utility = utility;
    }
  }
  if (proposed.valid &&
      acceptsGroupProposal(individual.utility, proposed.utility))
    return proposed;
  return individual;
}

namespace detail {
bool validGuestGroup(const GuestGroup &group) noexcept {
  if (group.id == 0 || group.leader == 0 || group.members.size() < 2 ||
      group.members.size() > 1024 || group.cohesion < 0 ||
      group.cohesion > 10000 || group.cohesionRadiusTiles < 0 ||
      group.cohesionRadiusTiles > 1024 || group.sharedItinerary.size() > 128)
    return false;

  std::unordered_set<EntityId> members;
  bool leaderPresent = false;
  for (const auto member : group.members) {
    if (member == 0 || !members.insert(member).second)
      return false;
    leaderPresent = leaderPresent || member == group.leader;
  }
  if (!leaderPresent)
    return false;
  return std::all_of(group.sharedItinerary.begin(), group.sharedItinerary.end(),
                     [](GuestGoalClass goal) { return validGoalValue(goal); });
}

std::string serializeGuestGroup(const GuestGroup &group) {
  if (!validGuestGroup(group))
    throw std::invalid_argument("guest group state is invalid");
  std::ostringstream output;
  output << "HGG1 " << group.id << ' ' << group.leader << ' ' << group.cohesion
         << ' ' << group.cohesionRadiusTiles << ' ' << group.members.size();
  for (const auto member : group.members)
    output << ' ' << member;
  output << ' ' << group.sharedItinerary.size();
  for (const auto goal : group.sharedItinerary)
    output << ' ' << static_cast<int>(goal);
  return output.str();
}

GuestGroup deserializeGuestGroup(std::string_view archive) {
  if (archive.size() > 64 * 1024)
    throw std::invalid_argument("guest group archive is too large");
  std::istringstream input{std::string(archive)};
  std::string magic;
  GuestGroup group;
  std::size_t memberCount{}, itineraryCount{};
  input >> magic >> group.id >> group.leader >> group.cohesion >>
      group.cohesionRadiusTiles >> memberCount;
  if (!input || magic != "HGG1" || memberCount < 2 || memberCount > 1024)
    throw std::invalid_argument("invalid guest group archive");
  group.members.resize(memberCount);
  for (auto &member : group.members)
    input >> member;
  input >> itineraryCount;
  if (!input || itineraryCount > 128)
    throw std::invalid_argument("invalid guest group itinerary");
  group.sharedItinerary.resize(itineraryCount);
  for (auto &goal : group.sharedItinerary) {
    int value{};
    input >> value;
    if (!input || value < static_cast<int>(GuestGoalClass::ReachHotel) ||
        value > static_cast<int>(GuestGoalClass::LeaveHotel))
      throw std::invalid_argument("invalid guest group goal");
    goal = static_cast<GuestGoalClass>(value);
  }
  input >> std::ws;
  if (!input.eof() || !validGuestGroup(group))
    throw std::invalid_argument("invalid guest group archive");
  return group;
}
} // namespace detail

} // namespace hh::game
