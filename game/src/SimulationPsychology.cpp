#include "hh/game/Simulation.h"
#include "hh/game/GuestGoals.h"
#include "hh/game/GuestPsychology.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace hh::game {
namespace {
const ReservationView *findReservation(const SimulationView &view,
                                       EntityId guestId) noexcept {
  const auto found = std::find_if(
      view.reservations.begin(), view.reservations.end(),
      [&](const ReservationView &reservation) { return reservation.id == guestId; });
  return found == view.reservations.end() ? nullptr : &*found;
}

const PersonView *findActiveGuest(const SimulationView &view,
                                  EntityId guestId) noexcept {
  const auto found = std::find_if(
      view.people.begin(), view.people.end(), [&](const PersonView &person) {
        return person.kind == PersonKind::Guest && person.reservationId == guestId;
      });
  return found == view.people.end() ? nullptr : &*found;
}

void projectLegacyState(GuestPsychologySnapshot &snapshot,
                        const ReservationView &reservation,
                        const PersonView *person) noexcept {
  const int legacySatisfaction = static_cast<int>(std::clamp(
      std::lround(person ? person->satisfaction : reservation.satisfaction), 0L,
      100L));
  snapshot.satisfaction.room = legacySatisfaction;
  snapshot.satisfaction.service = legacySatisfaction;
  snapshot.satisfaction.cleanliness = legacySatisfaction;
  snapshot.satisfaction.food = legacySatisfaction;
  snapshot.satisfaction.amenities = legacySatisfaction;
  snapshot.satisfaction.convenience = legacySatisfaction;
  snapshot.satisfaction.value = legacySatisfaction;
  snapshot.satisfaction.arrivalDeparture = legacySatisfaction;
  snapshot.satisfaction.checkIn = legacySatisfaction;
  snapshot.satisfaction.noise = legacySatisfaction;
  snapshot.satisfaction.waits = legacySatisfaction;
  snapshot.satisfaction.expectationFit = legacySatisfaction;
  snapshot.satisfaction.overall = legacySatisfaction;
  snapshot.operational.serviceConfidence = legacySatisfaction;
  snapshot.operational.cleanlinessConfidence = legacySatisfaction;
  snapshot.operational.environmentComfort = legacySatisfaction;
  snapshot.operational.valuePerception = legacySatisfaction;
  if (person) {
    snapshot.needs.hunger = static_cast<int>(
        std::clamp(std::lround(person->hunger), 0L, 100L));
    snapshot.needs.energy =
        static_cast<int>(std::clamp(std::lround(person->rest), 0L, 100L));
  }
}
} // namespace

GuestPsychologySnapshot Simulation::guestPsychology(EntityId guestId) const {
  const auto current = view();
  const auto *reservation = findReservation(current, guestId);
  if (!reservation)
    throw std::invalid_argument("guest psychology state not found");

  GuestPsychology service(0);
  service.initializeGuest(guestId, reservation->profile);
  const auto *person = findActiveGuest(current, guestId);

  const int waitSeconds = person ? person->queueWaitSeconds
                                 : reservation->checkInWaitSeconds;
  const int tolerance = person && person->queueToleranceSeconds > 0
                            ? person->queueToleranceSeconds
                            : detail::queueToleranceFor(reservation->profile);
  if (waitSeconds > tolerance + 10 * 60) {
    ExperienceEvent event;
    event.type = ExperienceEventType::LongCheckInQueue;
    event.timestampSeconds = current.elapsedSeconds;
    event.locationId = reservation->roomId;
    event.category = ExperienceCategory::ArrivalDeparture;
    event.observedValue = waitSeconds;
    event.expectedValue = tolerance;
    event.rawImpact = -50;
    event.memorySalience = 10000;
    event.memoryHalfLifeHours = 24;
    event.complaintEligible = true;
    service.recordExperience(guestId, event);
  }

  auto snapshot = *service.snapshot(guestId);
  projectLegacyState(snapshot, *reservation, person);
  if (waitSeconds > tolerance + 10 * 60) {
    snapshot.satisfaction.checkIn = std::min(snapshot.satisfaction.checkIn, 20);
    snapshot.satisfaction.waits = std::min(snapshot.satisfaction.waits, 20);
    snapshot.satisfaction.arrivalDeparture =
        std::min(snapshot.satisfaction.arrivalDeparture, 20);
  }
  return snapshot;
}

GoalSelection
Simulation::chooseGuestGoal(EntityId guestId,
                            const GuestOpportunitySnapshot &opportunities) const {
  return hh::game::chooseGuestGoal(guestId, opportunities);
}

SatisfactionBreakdown
Simulation::finalizeStaySatisfaction(EntityId guestId) const {
  return guestPsychology(guestId).satisfaction;
}

} // namespace hh::game
