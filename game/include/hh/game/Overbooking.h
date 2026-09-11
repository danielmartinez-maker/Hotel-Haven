#pragma once

#include <cstdint>
#include <limits>
#include <map>
#include <string>
#include <string_view>

namespace hh::game {

enum class OverbookingAllowanceMode : std::uint8_t { Rooms, Percentage };

struct OverbookingPolicy {
  std::string roomCategory;
  int allowance{};
  std::int64_t relocationCompensationCents{};
  int startDay{};
  int endDay{std::numeric_limits<int>::max()};
  OverbookingAllowanceMode allowanceMode{OverbookingAllowanceMode::Rooms};
  int allowanceBasisPoints{};
  bool operator==(const OverbookingPolicy &) const = default;
};

struct OverbookingResult {
  bool ok{};
  std::string reason;
  explicit operator bool() const noexcept { return ok; }
};

enum class RecoveryAction : std::uint8_t {
  CategoryEquivalentRoom,
  FreeUpgrade,
  PaidUpgrade,
  AccelerateRoomRecovery,
  CompetitorRelocation,
  Unresolved
};

struct RecoveryContext {
  std::string roomCategory{"standard"};
  bool categoryEquivalentAvailable{};
  bool freeUpgradeAvailable{};
  bool paidUpgradeAvailable{};
  bool guestAcceptsPaidUpgrade{};
  bool accelerationFeasible{};
  bool competitorRelocationAvailable{};
};

struct RecoveryDecision {
  RecoveryAction action{RecoveryAction::Unresolved};
  std::int64_t compensationCents{};
  std::string reason;
  bool severeExperienceEvent{};
  std::string experienceEventCode;
};

class OverbookingSystem {
public:
  [[nodiscard]] OverbookingResult setPolicy(const OverbookingPolicy &policy);
  [[nodiscard]] int allowance(std::string_view category) const;
  [[nodiscard]] int allowance(std::string_view category, int day) const;
  [[nodiscard]] int allowance(std::string_view category, int day,
                              int physicalRooms) const;
  [[nodiscard]] RecoveryDecision chooseRecovery(const RecoveryContext &context) const;
  [[nodiscard]] const std::map<std::string, OverbookingPolicy> &policies() const noexcept;
  [[nodiscard]] std::string save() const;
  static OverbookingSystem load(std::string_view data);

private:
  std::map<std::string, OverbookingPolicy> policies_;
};

} // namespace hh::game
