#pragma once

#include "hh/game/Simulation.h"
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace hh::game {

struct GuestPreferenceState;

enum class GuestGoalClass : std::uint8_t {
  ReachHotel,
  CheckIn,
  ReachRoom,
  Sleep,
  Eat,
  Drink,
  Bathe,
  Work,
  Exercise,
  Swim,
  Socialize,
  Relax,
  AttendEvent,
  RequestService,
  ResolveComplaint,
  Checkout,
  LeaveHotel
};

struct GoalOpportunity {
  std::uint32_t stableGoalId{};
  GuestGoalClass goal{GuestGoalClass::Relax};
  int needScore{100};
  int preference{10000};
  int availability{10000};
  int timeCompatibility{10000};
  int budgetCompatibility{10000};
  int groupCompatibility{10000};
  int moodModifier{10000};
  int expectedTravelSeconds{};
  int expectedWaitSeconds{};
  bool mandatory{};

  bool operator==(const GoalOpportunity &) const = default;
};

struct GuestOpportunitySnapshot {
  std::vector<GoalOpportunity> opportunities;

  bool operator==(const GuestOpportunitySnapshot &) const = default;
};

struct GoalSelection {
  bool valid{};
  std::uint32_t stableGoalId{};
  GuestGoalClass goal{GuestGoalClass::Relax};
  std::int64_t utility{};

  bool operator==(const GoalSelection &) const = default;
};

struct GuestGroup {
  EntityId id{};
  EntityId leader{};
  std::vector<EntityId> members;
  int cohesion{10000};
  int cohesionRadiusTiles{};
  std::vector<GuestGoalClass> sharedItinerary;

  bool operator==(const GuestGroup &) const = default;
};

[[nodiscard]] std::int64_t scoreGuestGoal(const GoalOpportunity &opportunity) noexcept;
[[nodiscard]] GuestOpportunitySnapshot
applyGuestPreferences(const GuestPreferenceState &preferences,
                      const GuestOpportunitySnapshot &snapshot) noexcept;
[[nodiscard]] GoalSelection chooseGuestGoal(EntityId guestId,
                                            const GuestOpportunitySnapshot &snapshot) noexcept;
[[nodiscard]] bool acceptsGroupProposal(std::int64_t bestIndividualUtility,
                                        std::int64_t proposedGroupUtility) noexcept;
[[nodiscard]] GoalSelection
chooseGuestGroupMemberGoal(EntityId guestId, const GuestGroup &group,
                           const GuestOpportunitySnapshot &snapshot) noexcept;

namespace detail {
[[nodiscard]] bool validGuestGroup(const GuestGroup &group) noexcept;
[[nodiscard]] std::string serializeGuestGroup(const GuestGroup &group);
[[nodiscard]] GuestGroup deserializeGuestGroup(std::string_view archive);
} // namespace detail

} // namespace hh::game
