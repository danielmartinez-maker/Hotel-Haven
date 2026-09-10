#include "hh/game/GuestReviews.h"
#include "hh/game/Simulation.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace hh::game;

namespace {
void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

GuestMemory memory(ExperienceEventType type, ExperienceCategory category,
                   int valence, int magnitude, std::int64_t timestamp) {
  GuestMemory result;
  result.type = type;
  result.category = category;
  result.valence = valence;
  result.magnitude = magnitude;
  result.salience = 10000;
  result.halfLifeHours = 48;
  result.timestampSeconds = timestamp;
  return result;
}

void review_score_is_deterministic_and_uses_modeled_satisfaction() {
  GuestPsychologySnapshot high;
  high.guestId = 9001;
  high.satisfaction.overall = 92;
  GuestPsychologySnapshot low = high;
  low.satisfaction.overall = 45;
  const auto a = buildGuestReview(high, 42, 3600);
  const auto b = buildGuestReview(high, 42, 3600);
  const auto c = buildGuestReview(low, 42, 3600);
  require(a == b, "same review evidence produced nondeterministic review");
  require(a.score >= 1.0 && a.score <= 10.0 && c.score >= 1.0 &&
              c.score <= 10.0,
          "review score escaped the HMG-010 1.0-10.0 range");
  require(a.score > c.score,
          "review score ignored modeled overall satisfaction");
}

void review_text_uses_only_strongest_actual_memories() {
  GuestPsychologySnapshot psychology;
  psychology.guestId = 9002;
  psychology.satisfaction.overall = 72;
  psychology.memories = {
      memory(ExperienceEventType::GreatMeal, ExperienceCategory::Food, 1, 70,
             1000),
      memory(ExperienceEventType::DirtyBathroom,
             ExperienceCategory::Cleanliness, -1, 65, 1100),
      memory(ExperienceEventType::ElevatorDelay,
             ExperienceCategory::Convenience, -1, 5, 1200)};
  const auto review = buildGuestReview(psychology, 17, 1300);
  require(review.evidence.size() >= 2 && review.evidence.size() <= 3,
          "review did not select the strongest 1-3 material memories");
  require(std::find(review.evidence.begin(), review.evidence.end(),
                    ExperienceEventType::GreatMeal) != review.evidence.end() &&
              std::find(review.evidence.begin(), review.evidence.end(),
                        ExperienceEventType::DirtyBathroom) !=
                  review.evidence.end(),
          "review omitted stronger experienced events");
  require(std::find(review.evidence.begin(), review.evidence.end(),
                    ExperienceEventType::ElevatorDelay) == review.evidence.end(),
          "review promoted a trivial memory over stronger evidence");
  require(review.text.find("meal") != std::string::npos &&
              review.text.find("bathroom") != std::string::npos &&
              review.text.find("elevator") == std::string::npos,
          "review text was not grounded only in selected memory evidence");
}

void empty_memory_review_does_not_invent_an_incident() {
  GuestPsychologySnapshot psychology;
  psychology.guestId = 9003;
  psychology.satisfaction.overall = 70;
  const auto review = buildGuestReview(psychology, 99, 0);
  require(review.evidence.empty(),
          "empty-memory review invented supporting evidence");
  require(review.text == "The stay matched my overall experience.",
          "empty-memory review invented an unexperienced service incident");
}

void completed_critic_review_uses_archived_memory() {
  bool exercised = false;
  for (std::uint64_t seed = 40; seed < 168 && !exercised; ++seed) {
    auto sim = Simulation::tutorial(seed);
    require(sim.loadDefinitions(
                    R"({"baseDemand":100,"roomConditionLossPerDay":0,"initialLinen":500,"initialTowels":1000,"initialAmenities":500,"initialChemicals":500})")
                .ok,
            "critic review integration definitions rejected");
    for (const auto &room : sim.view().rooms)
      require(sim.setRoomRate(room.id, 50).ok,
              "critic review integration rate change failed");
    sim.step(3600);

    EntityId criticId{};
    for (const auto &reservation : sim.view().reservations)
      if (!reservation.completed &&
          reservation.profile.archetype == GuestArchetype::CriticReviewer) {
        criticId = reservation.id;
        break;
      }
    if (!criticId)
      continue;

    ExperienceEvent meal;
    meal.type = ExperienceEventType::GreatMeal;
    meal.timestampSeconds = sim.view().elapsedSeconds;
    meal.category = ExperienceCategory::Food;
    meal.rawImpact = 70;
    meal.memorySalience = 10000;
    meal.memoryHalfLifeHours = 120;
    require(sim.recordGuestExperience(criticId, meal).ok,
            "critic meal experience could not be recorded");

    sim.step(5 * 86400);
    const auto view = sim.view();
    const auto review = std::find_if(
        view.reviews.begin(), view.reviews.end(),
        [&](const ReviewView &candidate) { return candidate.reservationId == criticId; });
    require(review != view.reviews.end(),
            "completed critic stay did not emit its forced review");
    require(review->text.find("meal") != std::string::npos,
            "completed critic review ignored an authoritative meal memory");
    exercised = true;
  }
  require(exercised, "critic integration fixture did not find a critic guest");
}
} // namespace

int main() {
  try {
    review_score_is_deterministic_and_uses_modeled_satisfaction();
    review_text_uses_only_strongest_actual_memories();
    empty_memory_review_does_not_invent_an_incident();
    completed_critic_review_uses_archived_memory();
  } catch (const std::exception &error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
  std::cout << "All guest review tests passed\n";
  return 0;
}
