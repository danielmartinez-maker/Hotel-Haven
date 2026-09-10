#include "hh/game/Simulation.h"
#include "hh/game/GuestGoals.h"
#include "hh/game/GuestPsychology.h"
#include "hh/game/GuestPsychologyArchive.h"
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

void projectLegacySatisfaction(GuestPsychologySnapshot &snapshot,
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
}

void projectLiveNeeds(GuestPsychologySnapshot &snapshot,
                      const PersonView *person) noexcept {
  if (!person)
    return;
  snapshot.needs.hunger =
      static_cast<int>(std::clamp(std::lround(person->hunger), 0L, 100L));
  snapshot.needs.energy =
      static_cast<int>(std::clamp(std::lround(person->rest), 0L, 100L));
}

bool hasLongCheckInMemory(const GuestPsychologySnapshot &snapshot) noexcept {
  return std::any_of(snapshot.memories.begin(), snapshot.memories.end(),
                     [](const GuestMemory &memory) {
                       return memory.type ==
                              ExperienceEventType::LongCheckInQueue;
                     });
}
} // namespace

GuestPsychologySnapshot Simulation::guestPsychology(EntityId guestId) const {
  const auto current = view();
  const auto *reservation = findReservation(current, guestId);
  if (!reservation)
    throw std::invalid_argument("guest psychology state not found");

  GuestPsychology service(0);
  const bool archived = !reservation->psychologyArchive.empty();
  if (archived) {
    auto restored =
        detail::deserializeGuestPsychology(reservation->psychologyArchive);
    if (restored.guestId != guestId || restored.profile != reservation->profile)
      throw std::invalid_argument("guest psychology archive identity mismatch");
    service.restoreGuest(restored);
  } else {
    service.initializeGuest(guestId, reservation->profile);
  }
  const auto *person = findActiveGuest(current, guestId);

  auto currentPsychology = *service.snapshot(guestId);
  const int waitSeconds = person ? person->queueWaitSeconds
                                 : reservation->checkInWaitSeconds;
  const int tolerance = person && person->queueToleranceSeconds > 0
                            ? person->queueToleranceSeconds
                            : detail::queueToleranceFor(reservation->profile);
  if (waitSeconds > tolerance + 10 * 60 &&
      !hasLongCheckInMemory(currentPsychology)) {
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
    currentPsychology = *service.snapshot(guestId);
  }

  if (!archived)
    projectLegacySatisfaction(currentPsychology, *reservation, person);
  projectLiveNeeds(currentPsychology, person);
  if (waitSeconds > tolerance + 10 * 60) {
    currentPsychology.satisfaction.checkIn =
        std::min(currentPsychology.satisfaction.checkIn, 20);
    currentPsychology.satisfaction.waits =
        std::min(currentPsychology.satisfaction.waits, 20);
    currentPsychology.satisfaction.arrivalDeparture =
        std::min(currentPsychology.satisfaction.arrivalDeparture, 20);
  }
  return currentPsychology;
}

GoalSelection
Simulation::chooseGuestGoal(EntityId guestId,
                            const GuestOpportunitySnapshot &opportunities) const {
  try {
    const auto psychology = guestPsychology(guestId);
    return hh::game::chooseGuestGoal(
        guestId, applyGuestPreferences(psychology.preferences, opportunities));
  } catch (const std::invalid_argument &) {
    // Keep the stable selection helper usable for prospective/non-resident IDs
    // while applying authoritative preferences whenever guest state exists.
    return hh::game::chooseGuestGoal(guestId, opportunities);
  }
}

SatisfactionBreakdown
Simulation::finalizeStaySatisfaction(EntityId guestId) const {
  return guestPsychology(guestId).satisfaction;
}

} // namespace hh::game
