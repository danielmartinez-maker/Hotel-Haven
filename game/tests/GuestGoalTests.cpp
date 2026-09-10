#include "hh/game/GuestGoals.h"
#include "hh/game/GuestPsychology.h"
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

void business_preferences_materially_reorder_discretionary_goals() {
  GuestPsychology psychology(701);
  GuestProfileView profile;
  profile.archetype = GuestArchetype::BusinessTraveler;
  psychology.initializeGuest(5001, profile);
  const auto state = psychology.snapshot(5001);
  require(state.has_value(), "business goal preference fixture was not initialized");

  GuestOpportunitySnapshot snapshot;
  snapshot.opportunities = {opportunity(10, GuestGoalClass::Swim),
                            opportunity(20, GuestGoalClass::Work)};
  const auto neutral = chooseGuestGoal(5001, snapshot);
  require(neutral.valid && neutral.goal == GuestGoalClass::Swim,
          "neutral tie fixture did not begin with the stable-id winner");

  const auto personalized = applyGuestPreferences(state->preferences, snapshot);
  const auto selected = chooseGuestGoal(5001, personalized);
  require(selected.valid && selected.goal == GuestGoalClass::Work,
          "business preference vector did not materially favor work over swimming");
}

void preferences_do_not_weaken_mandatory_lifecycle_goals() {
  GuestPreferenceState preferences;
  preferences.pool = 0;
  GuestOpportunitySnapshot snapshot;
  auto work = opportunity(1, GuestGoalClass::Work, 0);
  auto mandatory = opportunity(99, GuestGoalClass::Checkout, 100);
  mandatory.mandatory = true;
  snapshot.opportunities = {work, mandatory};
  const auto personalized = applyGuestPreferences(preferences, snapshot);
  const auto selected = chooseGuestGoal(5002, personalized);
  require(selected.valid && selected.goal == GuestGoalClass::Checkout,
          "preference weighting altered a mandatory lifecycle goal");
}

void live_business_guest_uses_authoritative_preferences() {
  bool exercised = false;
  for (std::uint64_t seed = 40; seed < 96 && !exercised; ++seed) {
    auto simulation = Simulation::tutorial(seed);
    require(simulation.loadDefinitions(R"({"baseDemand":100})").ok,
            "live business goal fixture definitions rejected");
    for (const auto &room : simulation.view().rooms)
      require(simulation.setRoomRate(room.id, 50).ok,
              "live business goal fixture rate update failed");
    simulation.step(3600);

    for (const auto &reservation : simulation.view().reservations) {
      if (reservation.completed ||
          reservation.profile.archetype != GuestArchetype::BusinessTraveler)
        continue;
      GuestOpportunitySnapshot snapshot;
      snapshot.opportunities = {opportunity(10, GuestGoalClass::Swim),
                                opportunity(20, GuestGoalClass::Work)};
      const auto selected = simulation.chooseGuestGoal(reservation.id, snapshot);
      require(selected.valid && selected.goal == GuestGoalClass::Work,
              "live Simulation goal choice ignored authoritative guest preferences");
      exercised = true;
      break;
    }
  }
  require(exercised, "live business goal fixture did not find a business guest");
}

void group_proposal_uses_documented_thirty_percent_acceptance_band() {
  require(acceptsGroupProposal(10000, 7000),
          "proposal exactly 30 percent below best alternative was rejected");
  require(!acceptsGroupProposal(10000, 6999),
          "proposal outside 30 percent acceptance band was accepted");
}

void group_follower_accepts_shared_itinerary_within_band() {
  GuestGroup group;
  group.id = 88;
  group.leader = 1001;
  group.members = {1001, 1002};
  group.cohesion = 9000;
  group.cohesionRadiusTiles = 6;
  group.sharedItinerary = {GuestGoalClass::Socialize};

  auto relax = opportunity(10, GuestGoalClass::Relax, 40);
  auto socialize = opportunity(20, GuestGoalClass::Socialize, 40);
  socialize.preference = 8000;
  GuestOpportunitySnapshot snapshot;
  snapshot.opportunities = {relax, socialize};

  const auto individual = chooseGuestGoal(1002, snapshot);
  require(individual.valid && individual.goal == GuestGoalClass::Relax,
          "group acceptance fixture did not start with an individual alternative");
  const auto follower = chooseGuestGroupMemberGoal(1002, group, snapshot);
  require(follower.valid && follower.goal == GuestGoalClass::Socialize,
          "group follower rejected a shared itinerary within the 30 percent band");
  const auto leader = chooseGuestGroupMemberGoal(1001, group, snapshot);
  require(leader == individual,
          "group leader did not retain authority to propose its own best action");
}

void urgent_need_temporarily_splits_group_follower() {
  GuestGroup group;
  group.id = 89;
  group.leader = 2001;
  group.members = {2001, 2002};
  group.cohesion = 9000;
  group.cohesionRadiusTiles = 6;
  group.sharedItinerary = {GuestGoalClass::Socialize};

  auto socialize = opportunity(10, GuestGoalClass::Socialize, 60);
  auto urgent = opportunity(20, GuestGoalClass::Bathe, 14);
  GuestOpportunitySnapshot snapshot;
  snapshot.opportunities = {socialize, urgent};
  const auto selected = chooseGuestGroupMemberGoal(2002, group, snapshot);
  require(selected.valid && selected.goal == GuestGoalClass::Bathe,
          "critical need did not temporarily split a group follower");
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

void simulation_owns_and_persists_guest_groups() {
  auto simulation = Simulation::tutorial(702);
  require(simulation.loadDefinitions(R"({"baseDemand":100})").ok,
          "group persistence fixture definitions rejected");
  for (const auto &room : simulation.view().rooms)
    require(simulation.setRoomRate(room.id, 50).ok,
            "group persistence fixture rate update failed");
  simulation.step(3600);

  std::vector<EntityId> guests;
  for (const auto &reservation : simulation.view().reservations)
    if (!reservation.completed && !reservation.walkedRelocated) {
      guests.push_back(reservation.id);
      if (guests.size() == 2)
        break;
    }
  require(guests.size() == 2,
          "group persistence fixture did not produce two active guests");

  GuestGroup spec;
  spec.leader = guests.front();
  spec.members = guests;
  spec.cohesion = 8500;
  spec.cohesionRadiusTiles = 6;
  spec.sharedItinerary = {GuestGoalClass::Eat, GuestGoalClass::AttendEvent};
  const auto created = simulation.createGuestGroup(spec);
  require(created.ok && created.id != 0,
          "simulation rejected a valid authoritative guest group");

  const auto before = simulation.guestGroupsSnapshot();
  require(before.size() == 1 && before.front().id == created.id &&
              before.front().leader == guests.front() &&
              before.front().members == guests &&
              before.front().cohesion == 8500 &&
              before.front().cohesionRadiusTiles == 6 &&
              before.front().sharedItinerary == spec.sharedItinerary,
          "simulation group snapshot lost authoritative group state");

  const auto loaded = Simulation::load(simulation.save());
  require(loaded.guestGroupsSnapshot() == before,
          "authoritative guest group state changed across save/load");
  require(loaded.save() == simulation.save(),
          "guest group save was not byte-stable after round trip");
}
} // namespace

int main() {
  try {
    equal_scores_use_stable_goal_id_tie_break();
    mandatory_lifecycle_goal_overrides_discretionary_utility();
    distance_and_wait_reduce_discretionary_utility();
    business_preferences_materially_reorder_discretionary_goals();
    preferences_do_not_weaken_mandatory_lifecycle_goals();
    live_business_guest_uses_authoritative_preferences();
    group_proposal_uses_documented_thirty_percent_acceptance_band();
    group_follower_accepts_shared_itinerary_within_band();
    urgent_need_temporarily_splits_group_follower();
    critical_need_can_override_group_incompatibility();
    group_state_is_value_stable_and_tracks_shared_itinerary();
    simulation_owns_and_persists_guest_groups();
  } catch (const std::exception &error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
  std::cout << "All guest goal/group tests passed\n";
  return 0;
}
