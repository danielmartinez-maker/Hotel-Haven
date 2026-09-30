#include "hh/game/GuestExperience.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

using namespace hh::game;

static void require(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}

static GuestExperienceEvent makeEvent(std::uint64_t eventId,
                                      EntityId incidentId,
                                      GuestExperienceEventType type,
                                      GuestCategory category,
                                      double rawImpact,
                                      bool complaintEligible,
                                      std::string statement) {
  GuestExperienceEvent event;
  event.eventId = eventId;
  event.incidentId = incidentId;
  event.type = type;
  event.timestampSeconds = 0;
  event.locationId = 10;
  event.category = category;
  event.observedValue = 40.0;
  event.expectedValue = 70.0;
  event.rawImpact = rawImpact;
  event.salience = 0.8;
  event.complaintEligible = complaintEligible;
  event.reviewStatement = std::move(statement);
  return event;
}

static void memory_contribution_uses_valence_salience_and_half_life() {
  GuestMemory memory;
  memory.valence = -1.0;
  memory.magnitude = 80.0;
  memory.salience = 0.5;
  memory.decayHalfLifeHours = 12.0;
  require(std::abs(guestMemoryContribution(memory, 12.0) + 20.0) < 1e-12,
          "memory contribution did not apply valence, salience, and half-life");
}

static void critical_memories_keep_in_stay_weight() {
  GuestMemory memory;
  memory.valence = -1.0;
  memory.magnitude = 90.0;
  memory.salience = 0.8;
  memory.decayHalfLifeHours = 6.0;
  memory.critical = true;
  const auto current = guestMemoryContribution(memory, 0.0);
  require(std::abs(guestMemoryContribution(memory, 72.0) - current) < 1e-12,
          "critical memory decayed during the guest's stay");
}

static void resolved_memory_and_complaint_are_updated_once() {
  GuestProfile profile;
  profile.sensitivities.service = 0.8;
  GuestExperienceState state;
  auto incident = makeEvent(1, 501, GuestExperienceEventType::StaffRudeness,
                            GuestCategory::Service, -50.0, true,
                            "The receptionist spoke rudely.");
  const auto created = applyGuestExperience(
      profile, state, incident, defaultGuestExperienceDefinitions(), 100, 200);
  require(created.memoryAdded && created.complaintCreated,
          "eligible incident did not create a memory and complaint");
  require(state.memories.size() == 1 && state.complaints.size() == 1,
          "incident memory or complaint was duplicated");
  require(state.events.size() == 1 && state.events.front().observedValue == 40.0 &&
              state.events.front().expectedValue == 70.0,
          "recorded event discarded its observed or expected values");

  auto recovery = makeEvent(2, 502, GuestExperienceEventType::Recovery,
                            GuestCategory::Service, 10.0, false,
                            "The staff offered a complimentary drink.");
  recovery.resolvesIncidentId = 501;
  recovery.resolvedMagnitudeReduction = 15.0;
  recovery.resolved = true;
  const auto resolved = applyGuestExperience(
      profile, state, recovery, defaultGuestExperienceDefinitions(), 101, 0);
  require(resolved.memoryResolved && resolved.complaintResolved,
          "recovery did not resolve the linked memory and complaint");
  require(state.events.size() == 2 && state.memories.size() == 2 &&
              state.memories.front().magnitude == 35.0,
          "recovery did not reduce the unresolved negative memory once");

  const auto repeated = applyGuestExperience(
      profile, state, recovery, defaultGuestExperienceDefinitions(), 102, 0);
  require(repeated.duplicate && state.events.size() == 2 &&
              state.memories.size() == 2 &&
              state.memories.front().magnitude == 35.0,
          "repeated recovery event changed a resolved incident twice");
}

static void complaint_thresholds_and_duplicate_incidents_are_enforced() {
  GuestProfile profile;
  profile.sensitivities.service = 0.8;
  GuestExperienceState state;
  const auto definitions = defaultGuestExperienceDefinitions();

  auto belowThreshold = makeEvent(1, 700, GuestExperienceEventType::LongCheckInQueue,
                                 GuestCategory::Service, -24.0, true,
                                 "The check-in line was slow.");
  const auto below = applyGuestExperience(profile, state, belowThreshold,
                                          definitions, 300, 400);
  require(!below.complaintCreated && state.complaints.empty(),
          "complaint was created below the unresolved-magnitude threshold");

  auto eligible = makeEvent(2, 701, GuestExperienceEventType::DirtyBathroom,
                            GuestCategory::Cleanliness, -30.0, true,
                            "The bathroom was not clean.");
  profile.sensitivities.cleanliness = 0.6;
  const auto first = applyGuestExperience(profile, state, eligible, definitions,
                                          301, 401);
  require(first.complaintCreated && state.complaints.size() == 1,
          "sensitive guest did not complain about an eligible incident");
  require(state.complaints.front().urgency == GuestComplaintUrgency::Low,
          "low-severity complaint received the wrong urgency");

  eligible.eventId = 3;
  const auto duplicateIncident = applyGuestExperience(
      profile, state, eligible, definitions, 302, 402);
  require(!duplicateIncident.complaintCreated && state.complaints.size() == 1,
          "the same incident created more than one complaint");

  GuestExperienceState emptyState;
  GuestExperienceEvent empty;
  const auto rejected = applyGuestExperience(profile, emptyState, empty,
                                             definitions, 500, 501);
  require(!rejected.accepted && emptyState.memories.empty(),
          "empty or unidentified event created an unsupported memory");
}

static void recovery_effectiveness_uses_hospitality_forgiveness_and_response_time() {
  GuestMemory memory;
  memory.valence = -1.0;
  memory.magnitude = 60.0;
  GuestRecoveryContext context;
  context.staffHospitality = 0.8;
  context.guestForgiveness = 0.75;
  context.responseMinutes = 10.0;
  context.complaintUrgency = GuestComplaintUrgency::Medium;
  context.issueFixed = true;

  const auto outcome = calculateGuestRecovery(
      memory, GuestRecoveryOption::PartialRoomRefund, context,
      defaultGuestExperienceDefinitions());
  require(outcome.has_value(), "valid partial refund recovery was rejected");
  require(std::abs(outcome->magnitudeReduction - 15.0) < 1e-12,
          "recovery ignored hospitality, forgiveness, or response speed");
  require(std::abs(outcome->responseSpeedFactor - 1.0) < 1e-12,
          "ten-minute response used the wrong speed factor");

  context.responseMinutes = 3.0;
  const auto fast = calculateGuestRecovery(
      memory, GuestRecoveryOption::PartialRoomRefund, context,
      defaultGuestExperienceDefinitions());
  require(fast && std::abs(fast->responseSpeedFactor - 1.25) < 1e-12,
          "response within five minutes missed the speed bonus");
}

static void review_probability_and_critic_modifier_are_deterministic() {
  GuestProfile critic;
  critic.archetype = GuestArchetype::CriticReviewer;
  GuestExperienceState state;
  state.categoryStartingSatisfaction.fill(72.0);
  state.categorySatisfaction.fill(72.0);
  auto definitions = defaultGuestExperienceDefinitions();
  definitions.reviewGenerationProbability = 0.0;
  std::uint64_t firstRandom = 123456;
  std::uint64_t secondRandom = 123456;
  const auto first = generateGuestReview(critic, state, 2, firstRandom,
                                         definitions);
  const auto repeated = generateGuestReview(critic, state, 2, secondRandom,
                                           definitions);
  require(first.generated && first == repeated,
          "critic review or its score was not deterministic");
  require(first.reputationImpactMultiplier == 5.0,
          "critic review did not receive its reputation impact multiplier");
  require(first.rating >= 1.0 && first.rating <= 10.0 &&
              std::abs(first.overallSatisfaction - 72.0) < 1e-12,
          "review rating and satisfaction were not kept as separate scales");
}

static void review_text_uses_only_actual_memory_statements() {
  GuestProfile profile;
  GuestExperienceState state;
  auto definitions = defaultGuestExperienceDefinitions();
  definitions.reviewGenerationProbability = 1.0;

  const auto positive = makeEvent(
      1, 801, GuestExperienceEventType::FreeUpgrade, GuestCategory::Room,
      25.0, false, "The suite upgrade made our stay special.");
  const auto negative = makeEvent(
      2, 802, GuestExperienceEventType::SlowRoomService,
      GuestCategory::Service, -35.0, false,
      "Room service took much longer than promised.");
  require(applyGuestExperience(profile, state, positive, definitions, 901, 0)
              .memoryAdded,
          "recorded positive experience did not create review memory");
  require(applyGuestExperience(profile, state, negative, definitions, 902, 0)
              .memoryAdded,
          "recorded negative experience did not create review memory");

  std::uint64_t randomState = 17;
  const auto review = generateGuestReview(profile, state, 1, randomState,
                                          definitions);
  require(review.generated &&
              review.text.find("The suite upgrade made our stay special.") !=
                  std::string::npos &&
              review.text.find("Room service took much longer than promised.") !=
                  std::string::npos,
          "review omitted recorded guest experiences");
  require(review.text.find("incredible spa") == std::string::npos &&
              review.text.find("airport shuttle") == std::string::npos,
          "review invented an unrecorded service or incident");
}

int main() {
  try {
    memory_contribution_uses_valence_salience_and_half_life();
    critical_memories_keep_in_stay_weight();
    resolved_memory_and_complaint_are_updated_once();
    complaint_thresholds_and_duplicate_incidents_are_enforced();
    recovery_effectiveness_uses_hospitality_forgiveness_and_response_time();
    review_probability_and_critic_modifier_are_deterministic();
    review_text_uses_only_actual_memory_statements();
  } catch (const std::exception &error) {
    std::cerr << "Guest experience test failed: " << error.what() << '\n';
    return 1;
  }
  return 0;
}
