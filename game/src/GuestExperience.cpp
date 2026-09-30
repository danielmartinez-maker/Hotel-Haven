#include "hh/game/GuestExperience.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace hh::game {
namespace {

constexpr double kDefaultCategoryWeights[GuestCategoryCount] = {
    0.28, 0.16, 0.24, 0.10, 0.08, 0.08, 0.06, 0.05, 0.03};

bool validCategory(GuestCategory value) noexcept {
  return static_cast<std::size_t>(value) < GuestCategoryCount;
}

bool validEventType(GuestExperienceEventType value) noexcept {
  return static_cast<std::size_t>(value) < GuestExperienceEventTypeCount;
}

bool validRecoveryOption(GuestRecoveryOption value) noexcept {
  return static_cast<std::size_t>(value) < GuestRecoveryOptionCount;
}

bool validUrgency(GuestComplaintUrgency value) noexcept {
  return static_cast<std::size_t>(value) <
         static_cast<std::size_t>(GuestComplaintUrgency::Count);
}

bool finiteRange(double value, double low, double high) noexcept {
  return std::isfinite(value) && value >= low && value <= high;
}

bool hasTrait(const GuestProfile &profile, GuestTrait trait) {
  return std::find(profile.traits.begin(), profile.traits.end(), trait) !=
         profile.traits.end();
}

double categorySensitivity(const GuestProfile &profile,
                           GuestCategory category) noexcept {
  switch (category) {
  case GuestCategory::Room:
    return profile.sensitivities.comfort;
  case GuestCategory::Cleanliness:
    return profile.sensitivities.cleanliness;
  case GuestCategory::Service:
    return profile.sensitivities.service;
  case GuestCategory::Food:
    return profile.sensitivities.food;
  case GuestCategory::Amenities:
    return profile.sensitivities.comfort;
  case GuestCategory::Quiet:
    return profile.sensitivities.noise;
  case GuestCategory::Convenience:
    return profile.sensitivities.service;
  case GuestCategory::Value:
    return profile.sensitivities.price;
  case GuestCategory::ArrivalDeparture:
    return profile.sensitivities.service;
  case GuestCategory::Count:
    return 0.0;
  }
  return 0.0;
}

GuestComplaintUrgency urgencyForMagnitude(double magnitude) noexcept {
  if (magnitude >= 80.0)
    return GuestComplaintUrgency::Critical;
  if (magnitude >= 60.0)
    return GuestComplaintUrgency::High;
  if (magnitude >= 40.0)
    return GuestComplaintUrgency::Medium;
  return GuestComplaintUrgency::Low;
}

bool validEvent(const GuestExperienceEvent &event,
                const GuestExperienceDefinitions &definitions) noexcept {
  return event.eventId != 0 && validEventType(event.type) &&
         event.timestampSeconds >= 0 && event.locationId != 0 &&
         (!event.sourceEntityId || *event.sourceEntityId != 0) &&
         validCategory(event.category) &&
         finiteRange(event.observedValue, 0.0, 100.0) &&
         finiteRange(event.expectedValue, 0.0, 100.0) &&
         finiteRange(event.rawImpact, -100.0, 100.0) &&
         finiteRange(event.salience, 0.0, 1.0) &&
         (!event.resolvesIncidentId || *event.resolvesIncidentId != 0) &&
         finiteRange(event.resolvedMagnitudeReduction, 0.0, 100.0) &&
         event.reviewStatement.size() <= 512 &&
         finiteRange(definitions.memoryHalfLifeHours[
                         static_cast<std::size_t>(event.type)],
                     0.001, 87600.0) &&
         (!event.complaintEligible || event.incidentId != 0);
}

double responseSpeedFactor(double responseMinutes) noexcept {
  if (responseMinutes <= 5.0)
    return 1.25;
  if (responseMinutes <= 15.0)
    return 1.0;
  if (responseMinutes <= 30.0)
    return 0.80;
  if (responseMinutes <= 60.0)
    return 0.60;
  return 0.40;
}

double unitDraw(std::uint64_t &state) noexcept {
  constexpr double unitScale = 1.0 / 9007199254740992.0;
  return static_cast<double>(drawGuestRandom(state) >> 11) * unitScale;
}

bool hasReviewStatement(const std::string &statement) noexcept {
  return std::any_of(statement.begin(), statement.end(), [](unsigned char ch) {
    return ch != ' ' && ch != '\t' && ch != '\n' && ch != '\r' &&
           ch != '\f' && ch != '\v';
  });
}

double contributionAt(const GuestMemory &memory,
                      std::int64_t timestampSeconds) noexcept {
  double ageHours = 0.0;
  if (timestampSeconds > memory.timestampSeconds) {
    ageHours = (static_cast<double>(timestampSeconds) -
                static_cast<double>(memory.timestampSeconds)) /
               3600.0;
  }
  return guestMemoryContribution(memory, ageHours);
}

void updateCategoryScores(GuestExperienceState &state,
                          std::int64_t timestampSeconds) noexcept {
  for (std::size_t i = 0; i < GuestCategoryCount; ++i) {
    double score = finiteRange(state.categoryStartingSatisfaction[i], 0.0, 100.0)
                       ? state.categoryStartingSatisfaction[i]
                       : 70.0;
    for (const auto &memory : state.memories) {
      if (static_cast<std::size_t>(memory.category) == i)
        score += contributionAt(memory, timestampSeconds);
    }
    state.categorySatisfaction[i] = std::clamp(score, 0.0, 100.0);
  }
}

} // namespace

GuestExperienceDefinitions defaultGuestExperienceDefinitions() {
  GuestExperienceDefinitions definitions;
  double weightTotal = 0.0;
  for (std::size_t i = 0; i < GuestCategoryCount; ++i) {
    definitions.categoryWeights[i] = kDefaultCategoryWeights[i];
    weightTotal += kDefaultCategoryWeights[i];
  }
  for (auto &weight : definitions.categoryWeights)
    weight /= weightTotal;

  definitions.memoryHalfLifeHours.fill(12.0);
  definitions.memoryHalfLifeHours[static_cast<std::size_t>(
      GuestExperienceEventType::LongCheckInQueue)] = 6.0;
  definitions.memoryHalfLifeHours[static_cast<std::size_t>(
      GuestExperienceEventType::RoomNotReady)] = 72.0;
  definitions.memoryHalfLifeHours[static_cast<std::size_t>(
      GuestExperienceEventType::FreeUpgrade)] = 48.0;
  definitions.memoryHalfLifeHours[static_cast<std::size_t>(
      GuestExperienceEventType::DirtyBathroom)] = 12.0;
  definitions.memoryHalfLifeHours[static_cast<std::size_t>(
      GuestExperienceEventType::QuickMaintenanceRecovery)] = 48.0;
  definitions.memoryHalfLifeHours[static_cast<std::size_t>(
      GuestExperienceEventType::SlowRoomService)] = 6.0;
  definitions.memoryHalfLifeHours[static_cast<std::size_t>(
      GuestExperienceEventType::ElevatorDelay)] = 6.0;
  definitions.memoryHalfLifeHours[static_cast<std::size_t>(
      GuestExperienceEventType::SecurityIncident)] = 72.0;
  definitions.recoveryMagnitudeReduction = {10.0, 15.0, 25.0, 35.0, 45.0};
  return definitions;
}

bool validateGuestExperienceDefinitions(
    const GuestExperienceDefinitions &definitions, std::string *error) {
  if (error)
    error->clear();
  const auto fail = [&](const char *message) {
    if (error)
      *error = message;
    return false;
  };

  double weightTotal = 0.0;
  for (const auto weight : definitions.categoryWeights) {
    if (!std::isfinite(weight) || weight < 0.0)
      return fail("guest experience category weight is invalid");
    weightTotal += weight;
  }
  if (!std::isfinite(weightTotal) || weightTotal <= 0.0)
    return fail("guest experience category weights have no positive sum");
  for (const auto halfLife : definitions.memoryHalfLifeHours)
    if (!finiteRange(halfLife, 0.001, 87600.0))
      return fail("guest memory half-life is out of range");
  for (const auto reduction : definitions.recoveryMagnitudeReduction)
    if (!finiteRange(reduction, 0.0, 100.0))
      return fail("guest recovery reduction is out of range");
  if (!finiteRange(definitions.complaintMagnitudeThreshold, 0.0, 100.0) ||
      !finiteRange(definitions.reviewGenerationProbability, 0.0, 1.0) ||
      !finiteRange(definitions.lowSatisfactionReviewBonus, 0.0, 1.0) ||
      !finiteRange(definitions.highSatisfactionReviewBonus, 0.0, 1.0) ||
      !finiteRange(definitions.reviewScoreNoiseRange, 0.0, 1.0) ||
      !finiteRange(definitions.criticReputationImpactMultiplier, 0.0, 100.0))
    return fail("guest experience threshold or review tuning is invalid");
  return true;
}

double guestMemoryContribution(const GuestMemory &memory,
                               double ageHours) noexcept {
  if (!finiteRange(memory.valence, -1.0, 1.0) ||
      !finiteRange(memory.magnitude, 0.0, 100.0) ||
      !finiteRange(memory.salience, 0.0, 1.0) || !std::isfinite(ageHours) ||
      ageHours < 0.0 || !finiteRange(memory.decayHalfLifeHours, 0.001, 87600.0))
    return 0.0;
  const double fullWeight = memory.valence * memory.magnitude * memory.salience;
  if (memory.critical)
    return fullWeight;
  return fullWeight * std::exp2(-ageHours / memory.decayHalfLifeHours);
}

double guestOverallSatisfaction(const GuestExperienceState &state,
                                std::int64_t currentTimestampSeconds,
                                const GuestExperienceDefinitions &definitions)
    noexcept {
  if (currentTimestampSeconds < 0)
    return 70.0;
  double totalWeight = 0.0;
  for (const auto weight : definitions.categoryWeights) {
    if (!std::isfinite(weight) || weight < 0.0)
      return 70.0;
    totalWeight += weight;
  }
  if (!std::isfinite(totalWeight) || totalWeight <= 0.0)
    return 70.0;

  double overall = 0.0;
  for (std::size_t category = 0; category < GuestCategoryCount; ++category) {
    double score = finiteRange(state.categoryStartingSatisfaction[category],
                               0.0, 100.0)
                       ? state.categoryStartingSatisfaction[category]
                       : 70.0;
    for (const auto &memory : state.memories) {
      if (!validCategory(memory.category) ||
          static_cast<std::size_t>(memory.category) != category)
        continue;
      score += contributionAt(memory, currentTimestampSeconds);
    }
    score = std::clamp(score, 0.0, 100.0);
    overall += score * (definitions.categoryWeights[category] / totalWeight);
  }
  return std::clamp(overall, 0.0, 100.0);
}

GuestExperienceResult applyGuestExperience(
    const GuestProfile &profile, GuestExperienceState &state,
    const GuestExperienceEvent &event,
    const GuestExperienceDefinitions &definitions, EntityId memoryId,
    EntityId complaintId) {
  GuestExperienceResult result;
  if (!validateGuestExperienceDefinitions(definitions) ||
      !validEvent(event, definitions))
    return result;
  if (std::any_of(state.events.begin(), state.events.end(),
                  [&](const GuestExperienceEvent &recorded) {
                    return recorded.eventId == event.eventId;
                  })) {
    result.duplicate = true;
    return result;
  }

  const bool addsMemory = event.rawImpact != 0.0;
  if (addsMemory &&
      (memoryId == 0 || std::any_of(state.memories.begin(), state.memories.end(),
                                    [memoryId](const GuestMemory &memory) {
                                      return memory.id == memoryId;
                                    })))
    return result;

  GuestMemory *resolvedMemory = nullptr;
  if (event.resolvesIncidentId) {
    const auto found = std::find_if(
        state.memories.begin(), state.memories.end(), [&](GuestMemory &memory) {
          return memory.incidentId == *event.resolvesIncidentId;
        });
    if (found == state.memories.end())
      return result;
    resolvedMemory = &*found;
  }

  result.accepted = true;
  if (resolvedMemory && !resolvedMemory->resolved) {
    const double before = resolvedMemory->magnitude;
    resolvedMemory->magnitude =
        std::max(0.0, resolvedMemory->magnitude -
                          event.resolvedMagnitudeReduction);
    resolvedMemory->resolved =
        event.resolved || resolvedMemory->magnitude <= 1e-9;
    result.memoryResolved =
        resolvedMemory->resolved || resolvedMemory->magnitude < before;
  }
  if (event.resolved ||
      (resolvedMemory && resolvedMemory->magnitude <= 1e-9)) {
    const EntityId incident = event.resolvesIncidentId
                                  ? *event.resolvesIncidentId
                                  : event.incidentId;
    for (auto &complaint : state.complaints) {
      if (complaint.incidentId == incident && complaint.open) {
        complaint.open = false;
        result.complaintResolved = true;
      }
    }
  }

  if (addsMemory) {
    GuestMemory memory;
    memory.id = memoryId;
    memory.eventId = event.eventId;
    memory.incidentId = event.incidentId;
    memory.type = event.type;
    memory.timestampSeconds = event.timestampSeconds;
    memory.locationId = event.locationId;
    memory.sourceEntityId = event.sourceEntityId;
    memory.category = event.category;
    memory.valence = event.rawImpact > 0.0 ? 1.0 : -1.0;
    memory.magnitude = std::abs(event.rawImpact);
    memory.salience = event.salience;
    memory.decayHalfLifeHours = definitions.memoryHalfLifeHours[
        static_cast<std::size_t>(event.type)];
    memory.critical = event.critical;
    memory.resolved = event.resolved;
    memory.reviewStatement = event.reviewStatement;
    state.memories.push_back(memory);
    result.memoryAdded = true;
    result.memoryId = memoryId;

    const double complaintThreshold = std::clamp(
        definitions.complaintMagnitudeThreshold +
            (std::isfinite(profile.complaintThresholdDelta)
                 ? profile.complaintThresholdDelta
                 : 0.0),
        0.0, 100.0);
    const double sensitivity = categorySensitivity(profile, event.category);
    const bool complaintProne = hasTrait(profile, GuestTrait::ComplaintProne);
    const bool alreadyReported = std::any_of(
        state.complaints.begin(), state.complaints.end(),
        [&](const GuestComplaint &complaint) {
          return complaint.incidentId == event.incidentId;
        });
    if (event.complaintEligible && !event.resolved && event.rawImpact < 0.0 &&
        event.incidentId != 0 && memory.magnitude >= complaintThreshold &&
        (sensitivity >= 0.55 || memory.magnitude >= 50.0 || complaintProne) &&
        !alreadyReported && complaintId != 0 &&
        std::none_of(state.complaints.begin(), state.complaints.end(),
                     [complaintId](const GuestComplaint &complaint) {
                       return complaint.id == complaintId;
                     })) {
      GuestComplaint complaint;
      complaint.id = complaintId;
      complaint.incidentId = event.incidentId;
      complaint.memoryId = memoryId;
      complaint.urgency = urgencyForMagnitude(memory.magnitude);
      state.complaints.push_back(complaint);
      result.complaintCreated = true;
      result.complaintId = complaintId;
      result.complaintUrgency = complaint.urgency;
    }
  }

  state.events.push_back(event);
  updateCategoryScores(state, event.timestampSeconds);
  result.overallSatisfaction = guestOverallSatisfaction(
      state, event.timestampSeconds, definitions);
  return result;
}

std::optional<GuestRecoveryOutcome> calculateGuestRecovery(
    const GuestMemory &memory, GuestRecoveryOption option,
    const GuestRecoveryContext &context,
    const GuestExperienceDefinitions &definitions) {
  if (!validateGuestExperienceDefinitions(definitions) ||
      !validRecoveryOption(option) || !validUrgency(context.complaintUrgency) ||
      !validEventType(memory.type) || !validCategory(memory.category) ||
      !finiteRange(memory.valence, -1.0, 1.0) || memory.valence >= 0.0 ||
      !finiteRange(memory.magnitude, 0.0, 100.0) ||
      !finiteRange(memory.salience, 0.0, 1.0) ||
      !finiteRange(memory.decayHalfLifeHours, 0.001, 87600.0) ||
      !finiteRange(context.staffHospitality, 0.0, 1.0) ||
      !finiteRange(context.guestForgiveness, 0.0, 1.0) ||
      !std::isfinite(context.responseMinutes) || context.responseMinutes < 0.0)
    return std::nullopt;

  switch (option) {
  case GuestRecoveryOption::ApologyOnly:
    if (!context.issueFixed)
      return std::nullopt;
    break;
  case GuestRecoveryOption::FreeDrink:
    if (context.complaintUrgency != GuestComplaintUrgency::Low &&
        context.complaintUrgency != GuestComplaintUrgency::Medium)
      return std::nullopt;
    break;
  case GuestRecoveryOption::RoomUpgrade:
    if (!context.superiorRoomAvailable)
      return std::nullopt;
    break;
  case GuestRecoveryOption::PartialRoomRefund:
  case GuestRecoveryOption::FullNightRefund:
  case GuestRecoveryOption::Count:
    break;
  }

  GuestRecoveryOutcome outcome;
  outcome.responseSpeedFactor = responseSpeedFactor(context.responseMinutes);
  outcome.effectiveness = context.staffHospitality * context.guestForgiveness *
                          outcome.responseSpeedFactor;
  const double base = definitions.recoveryMagnitudeReduction[
      static_cast<std::size_t>(option)];
  outcome.magnitudeReduction =
      std::min(memory.magnitude, base * outcome.effectiveness);
  outcome.remainingNegativeMagnitude =
      std::max(0.0, memory.magnitude - outcome.magnitudeReduction);
  return outcome;
}

GuestReview generateGuestReview(const GuestProfile &profile,
                                const GuestExperienceState &state,
                                int completedDay,
                                std::uint64_t &guestRandomState,
                                const GuestExperienceDefinitions &definitions) {
  GuestReview review;
  if (completedDay < 0 || !validateGuestExperienceDefinitions(definitions))
    return review;
  review.completedDay = completedDay;
  const auto timestampSeconds =
      static_cast<std::int64_t>(completedDay) * 24 * 60 * 60;
  review.overallSatisfaction =
      guestOverallSatisfaction(state, timestampSeconds, definitions);

  double probability = definitions.reviewGenerationProbability;
  if (review.overallSatisfaction <= 40.0)
    probability += definitions.lowSatisfactionReviewBonus;
  if (review.overallSatisfaction >= 90.0)
    probability += definitions.highSatisfactionReviewBonus;
  const bool critic = profile.archetype == GuestArchetype::CriticReviewer;
  if (critic)
    probability = 1.0;
  else if (profile.archetype == GuestArchetype::GroupTourTraveler &&
           !state.isGroupLeader)
    probability *= 0.5;
  review.generationProbability = std::clamp(probability, 0.0, 1.0);
  review.reputationImpactMultiplier =
      critic ? definitions.criticReputationImpactMultiplier : 1.0;

  const double reviewRoll = unitDraw(guestRandomState);
  if (reviewRoll >= review.generationProbability)
    return review;
  review.generated = true;
  const double scoreNoise =
      (unitDraw(guestRandomState) * 2.0 - 1.0) *
      definitions.reviewScoreNoiseRange;
  review.rating = std::clamp(
      1.0 + review.overallSatisfaction * 0.09 + scoreNoise, 1.0, 10.0);

  struct RankedMemory {
    const GuestMemory *memory{};
    double strength{};
    double contribution{};
  };
  std::vector<RankedMemory> ranked;
  double negativeWeight = profile.negativeMemoryReviewWeightMultiplier;
  if (!std::isfinite(negativeWeight) || negativeWeight < 0.0)
    negativeWeight = 1.0;
  negativeWeight = std::min(negativeWeight, 16.0);
  for (const auto &memory : state.memories) {
    if (!hasReviewStatement(memory.reviewStatement) ||
        !validCategory(memory.category))
      continue;
    double contribution = contributionAt(memory, timestampSeconds);
    if (contribution < 0.0)
      contribution *= negativeWeight;
    const double strength = std::abs(contribution);
    if (strength > 0.0)
      ranked.push_back({&memory, strength, contribution});
  }
  std::sort(ranked.begin(), ranked.end(), [](const RankedMemory &left,
                                              const RankedMemory &right) {
    if (left.strength != right.strength)
      return left.strength > right.strength;
    if (left.memory->id != right.memory->id)
      return left.memory->id < right.memory->id;
    return left.memory->eventId < right.memory->eventId;
  });
  std::vector<RankedMemory> selected;
  const auto addFirstWithSign = [&](bool positive) {
    const auto found = std::find_if(
        ranked.begin(), ranked.end(), [positive](const RankedMemory &candidate) {
          return positive ? candidate.contribution > 0.0
                          : candidate.contribution < 0.0;
        });
    if (found != ranked.end())
      selected.push_back(*found);
  };
  addFirstWithSign(true);
  addFirstWithSign(false);
  for (const auto &candidate : ranked) {
    if (selected.size() >= 3)
      break;
    const bool alreadySelected = std::any_of(
        selected.begin(), selected.end(), [&](const RankedMemory &chosen) {
          return chosen.memory == candidate.memory;
        });
    if (!alreadySelected)
      selected.push_back(candidate);
  }
  std::sort(selected.begin(), selected.end(), [](const RankedMemory &left,
                                                  const RankedMemory &right) {
    if (left.strength != right.strength)
      return left.strength > right.strength;
    if (left.memory->id != right.memory->id)
      return left.memory->id < right.memory->id;
    return left.memory->eventId < right.memory->eventId;
  });
  for (const auto &candidate : selected) {
    const auto &memory = *candidate.memory;
    if (!review.text.empty())
      review.text.push_back('\n');
    review.text += memory.reviewStatement;
    review.memoryIds.push_back(memory.id);
  }
  return review;
}

} // namespace hh::game
