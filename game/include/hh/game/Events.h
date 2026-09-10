#pragma once

#include "hh/game/ServiceTypes.h"
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace hh::game {

using EventBookingId = ServiceId;
using FunctionSpaceId = ServiceId;

enum class EventBlockReason : std::uint8_t {
  None,
  InvalidRequest,
  MissingFunctionSpace,
  InsufficientRoomCapacity,
  ServiceCapacity,
  StaffingCapacity,
  TimeConflict
};

enum class EventPhase : std::uint8_t {
  Confirmed,
  Setup,
  Service,
  Teardown,
  Completed,
  Cancelled
};

struct FunctionSpaceConfig {
  FunctionSpaceId id{};
  int capacity{};
  bool enabled{true};
};

struct EventRequest {
  FunctionSpaceId functionSpaceId{};
  std::int64_t startSecond{};
  int durationSeconds{3600};
  int attendees{};
  int mealServings{};
  int setupSeconds{1800};
  int teardownSeconds{1200};
  int requiredStaffUnits{};
  std::int64_t contractCents{};
};

struct EventQuote {
  bool confirmable{};
  EventBlockReason blockReason{EventBlockReason::None};
  FunctionSpaceId functionSpaceId{};
  std::int64_t quotedCents{};
};

struct EventBookingView {
  EventBookingId id{};
  FunctionSpaceId functionSpaceId{};
  std::int64_t startSecond{};
  std::int64_t endSecond{};
  int attendees{};
  int mealServings{};
  int requiredStaffUnits{};
  std::int64_t contractCents{};
  EventPhase phase{EventPhase::Confirmed};
};

struct EventWorkloadView {
  EventBookingId bookingId{};
  EventPhase phase{EventPhase::Confirmed};
  int staffUnits{};
  int mealServings{};
};

struct EventsSnapshot {
  std::int64_t elapsedSeconds{};
  std::vector<EventBookingView> bookings;
  std::vector<EventWorkloadView> workloads;
  std::int64_t revenueCents{};
};

class EventsSystem {
public:
  EventsSystem();
  ~EventsSystem();
  EventsSystem(EventsSystem &&) noexcept;
  EventsSystem &operator=(EventsSystem &&) noexcept;
  EventsSystem(const EventsSystem &);
  EventsSystem &operator=(const EventsSystem &);

  void addFunctionSpace(const FunctionSpaceConfig &space);
  void setFoodServiceCapacity(int servingsPerHour);
  void setStaffCapacity(int staffUnits);
  void setElapsedSeconds(std::int64_t elapsedSeconds);

  [[nodiscard]] EventQuote quoteEvent(const EventRequest &request) const;
  [[nodiscard]] EventBookingId confirmEvent(const EventRequest &request);
  [[nodiscard]] EventPhase phase(EventBookingId id) const;
  [[nodiscard]] std::vector<EventPhase> phaseHistory(EventBookingId id) const;
  void tickSecond();
  void tickSeconds(std::int64_t seconds);
  [[nodiscard]] EventsSnapshot snapshot() const;

  [[nodiscard]] std::string save() const;
  static EventsSystem load(std::string_view data);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace hh::game
