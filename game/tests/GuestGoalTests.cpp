#include "hh/game/GuestGoals.h"
#include <iostream>
#include <stdexcept>

using namespace hh::game;

namespace {
void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

GoalOpportunity opportunity(std::uint32_t stableId, GuestGoalClass goal,
                            int needScore = 40) {
  GoalOpportunity item;
  item.stableGoalId = stableId;
  item.goal = goal;
  item.needScore = needScore;
  item.preference = 10000;
  item.availability = 10000;
  item.timeCompatibility = 10000;
  item.budgetCompatibility = 10000;
  item.groupCompatibility = 10000;
  item.moodModifier = 10000;
  return item;
}

void equal_scores_use_stable_goal_id_tie_break() {
  GuestOpportunitySnapshot snapshot;
  snapshot.opportunities.push_back(opportunity(20, GuestGoalClass::Relax));
  snapshot.opportunities.push_back(opportunity(10, GuestGoalClass::Eat));
  const auto selected = chooseGuestGoal(1001, snapshot);
  require(selected.valid && selected.stableGoalId == 10 &&
              selected.goal == GuestGoalClass::Eat,
          "equal guest goal utilities did not use stable goal id tie-break");
}

void mandatory_lifecycle_goal_overrides_discretionary_utility() {
  GuestOpportunitySnapshot snapshot;
  auto discretionary = opportunity(1, GuestGoalClass::Eat, 0);
  auto mandatory = opportunity(99, GuestGoalClass::Checkout, 100);
  mandatory.mandatory = true;
  snapshot.opportunities = {discretionary, mandatory};
  const auto selected = chooseGuestGoal(1002, snapshot);
  require(selected.valid && selected.stableGoalId == 99 &&
              selected.goal == GuestGoalClass::Checkout,
          "mandatory lifecycle goal did not override discretionary utility");
}

void distance_and_wait_reduce_discretionary_utility() {
  auto near = opportunity(1, GuestGoalClass::Eat, 20);
  auto far = near;
  near.expectedTravelSeconds = 60;
  near.expectedWaitSeconds = 60;
  far.expectedTravelSeconds = 20 * 60;
  far.expectedWaitSeconds = 10 * 60;
  require(scoreGuestGoal(near) > scoreGuestGoal(far),
          "distance and expected wait did not reduce guest goal utility");
}

void group_proposal_uses_documented_thirty_percent_acceptance_band() {
  require(acceptsGroupProposal(10000, 7000),
          "proposal exactly 30 percent below best alternative was rejected");
  require(!acceptsGroupProposal(10000, 6999),
          "proposal outside 30 percent acceptance band was accepted");
}

void critical_need_can_override_group_incompatibility() {
  GuestOpportunitySnapshot snapshot;
  auto groupPlan = opportunity(1, GuestGoalClass::Socialize, 60);
  auto urgent = opportunity(2, GuestGoalClass::Bathe, 14);
  urgent.groupCompatibility = 0;
  snapshot.opportunities = {groupPlan, urgent};
  const auto selected = chooseGuestGoal(1003, snapshot);
  require(selected.valid && selected.stableGoalId == 2 &&
              selected.goal == GuestGoalClass::Bathe,
          "critical individual need could not override group incompatibility");
}

void group_state_is_value_stable_and_tracks_shared_itinerary() {
  GuestGroup group;
  group.id = 77;
  group.leader = 1001;
  group.members = {1001, 1002, 1003};
  group.cohesion = 8000;
  group.cohesionRadiusTiles = 6;
  group.sharedItinerary = {GuestGoalClass::Eat, GuestGoalClass::AttendEvent};
  const auto copy = group;
  require(copy == group && copy.members.size() == 3 &&
              copy.sharedItinerary.size() == 2,
          "guest group state did not retain authoritative itinerary/cohesion data");
}
} // namespace

int main() {
  try {
    equal_scores_use_stable_goal_id_tie_break();
    mandatory_lifecycle_goal_overrides_discretionary_utility();
    distance_and_wait_reduce_discretionary_utility();
    group_proposal_uses_documented_thirty_percent_acceptance_band();
    critical_need_can_override_group_incompatibility();
    group_state_is_value_stable_and_tracks_shared_itinerary();
  } catch (const std::exception &error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
  std::cout << "All guest goal/group tests passed\n";
  return 0;
}
