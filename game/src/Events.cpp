#include "hh/game/Events.h"

namespace hh::game {

struct EventsSystem::Impl {};
EventsSystem::EventsSystem() : impl_(std::make_unique<Impl>()) {}
EventsSystem::~EventsSystem() = default;
EventsSystem::EventsSystem(EventsSystem &&) noexcept = default;
EventsSystem &EventsSystem::operator=(EventsSystem &&) noexcept = default;
EventsSystem::EventsSystem(const EventsSystem &) : impl_(std::make_unique<Impl>()) {}
EventsSystem &EventsSystem::operator=(const EventsSystem &) { return *this; }
void EventsSystem::addFunctionSpace(const FunctionSpaceConfig &) {}
void EventsSystem::setFoodServiceCapacity(int) {}
void EventsSystem::setStaffCapacity(int) {}
void EventsSystem::setElapsedSeconds(std::int64_t) {}
EventQuote EventsSystem::quoteEvent(const EventRequest &) const { return {}; }
EventBookingId EventsSystem::confirmEvent(const EventRequest &) { return 0; }
EventPhase EventsSystem::phase(EventBookingId) const { return EventPhase::Cancelled; }
std::vector<EventPhase> EventsSystem::phaseHistory(EventBookingId) const { return {}; }
void EventsSystem::tickSecond() {}
void EventsSystem::tickSeconds(std::int64_t) {}
EventsSnapshot EventsSystem::snapshot() const { return {}; }
std::string EventsSystem::save() const { return {}; }
EventsSystem EventsSystem::load(std::string_view) { return {}; }

} // namespace hh::game
