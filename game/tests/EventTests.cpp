#include "hh/game/Events.h"
#include <stdexcept>
#include <vector>

using namespace hh::game;

static void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

static EventsSystem eventSystem() {
  EventsSystem events;
  events.addFunctionSpace({1, 100, true});
  events.addFunctionSpace({2, 250, true});
  events.setFoodServiceCapacity(1000);
  events.setStaffCapacity(20);
  return events;
}

static EventRequest standardEvent() {
  EventRequest request;
  request.functionSpaceId = 1;
  request.startSecond = 10;
  request.durationSeconds = 5;
  request.attendees = 60;
  request.mealServings = 60;
  request.setupSeconds = 3;
  request.teardownSeconds = 2;
  request.requiredStaffUnits = 6;
  request.contractCents = 125000;
  return request;
}

static void event_requires_space_and_service_feasibility() {
  auto events = eventSystem();
  auto oversized = standardEvent();
  oversized.attendees = 150;
  auto quote = events.quoteEvent(oversized);
  require(!quote.confirmable,
          "oversized event was confirmable in an undersized function room");
  require(quote.blockReason == EventBlockReason::InsufficientRoomCapacity,
          "capacity rejection reason was not explicit");

  auto serviceHeavy = standardEvent();
  serviceHeavy.mealServings = 2000;
  quote = events.quoteEvent(serviceHeavy);
  require(!quote.confirmable,
          "event exceeded configured F&B service capacity");
  require(quote.blockReason == EventBlockReason::ServiceCapacity,
          "F&B feasibility rejection reason was not explicit");
}

static void confirmed_event_generates_setup_service_and_teardown_workloads() {
  auto events = eventSystem();
  const auto request = standardEvent();
  const auto quote = events.quoteEvent(request);
  require(quote.confirmable, "valid event quote was not confirmable");
  const auto id = events.confirmEvent(request);
  require(id != 0, "valid event was not confirmed");
  events.tickSeconds(20);
  const std::vector<EventPhase> expected{
      EventPhase::Confirmed, EventPhase::Setup, EventPhase::Service,
      EventPhase::Teardown, EventPhase::Completed};
  require(events.phaseHistory(id) == expected,
          "event did not progress confirmed/setup/service/teardown/completed");
  require(events.snapshot().revenueCents == request.contractCents,
          "event contract revenue was not posted exactly once");
}

static void confirmed_event_reserves_its_function_space_window() {
  auto events = eventSystem();
  const auto first = standardEvent();
  require(events.confirmEvent(first) != 0, "first event was not confirmed");
  auto overlap = first;
  overlap.startSecond = 12;
  const auto quote = events.quoteEvent(overlap);
  require(!quote.confirmable, "overlapping event reused reserved function space");
  require(quote.blockReason == EventBlockReason::TimeConflict,
          "overlapping event did not expose TimeConflict");
}

static void event_state_round_trips_deterministically() {
  auto events = eventSystem();
  require(events.confirmEvent(standardEvent()) != 0,
          "event was not confirmed for persistence test");
  events.tickSeconds(11);
  const auto restored = EventsSystem::load(events.save());
  require(restored.save() == events.save(),
          "event save/load changed authoritative state");
}

int main() {
  event_requires_space_and_service_feasibility();
  confirmed_event_generates_setup_service_and_teardown_workloads();
  confirmed_event_reserves_its_function_space_window();
  event_state_round_trips_deterministically();
}
