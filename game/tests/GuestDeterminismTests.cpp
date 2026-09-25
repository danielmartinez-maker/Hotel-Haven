#include "hh/game/Simulation.h"
#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace hh::game;

namespace {
void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

void applyCampaignSetup(Simulation &simulation) {
  require(simulation
              .loadDefinitions(
                  R"({"baseDemand":4,"roomConditionLossPerDay":0,"initialLinen":600,"initialTowels":1200,"initialAmenities":600,"initialChemicals":600,"initialParts":100})")
              .ok,
          "determinism campaign definitions rejected");
  for (const auto &room : simulation.view().rooms)
    require(simulation.setRoomRate(room.id, 110).ok,
            "determinism campaign rate update failed");
}

void requireSameReviews(const SimulationView &first, const SimulationView &second) {
  require(first.reviews.size() == second.reviews.size(),
          "same-seed campaign produced a different review count");
  for (std::size_t index = 0; index < first.reviews.size(); ++index) {
    const auto &a = first.reviews[index];
    const auto &b = second.reviews[index];
    require(a.reservationId == b.reservationId && a.day == b.day &&
                std::abs(a.score - b.score) < 1e-12 && a.text == b.text,
            "same-seed campaign produced different review history");
  }
}

void booking_profile_stream_ignores_unrelated_entity_ids() {
  constexpr std::uint64_t seed = 0x424f4f4b494e47ULL;
  auto baseline = Simulation::tutorial(seed);
  auto perturbed = Simulation::tutorial(seed);

  require(perturbed
              .hireStaff({"Off-shift spare", PersonKind::Maintenance, 0, 1, 25})
              .ok,
          "could not create unrelated entity-id perturbation");

  require(baseline.loadDefinitions(R"({"baseDemand":100,"roomConditionLossPerDay":0})").ok &&
              perturbed.loadDefinitions(R"({"baseDemand":100,"roomConditionLossPerDay":0})").ok,
          "booking-stream fixture definitions rejected");
  for (const auto &room : baseline.view().rooms)
    require(baseline.setRoomRate(room.id, 50).ok,
            "baseline booking-stream rate rejected");
  for (const auto &room : perturbed.view().rooms)
    require(perturbed.setRoomRate(room.id, 50).ok,
            "perturbed booking-stream rate rejected");

  baseline.step(3600);
  perturbed.step(3600);
  const auto a = baseline.view();
  const auto b = perturbed.view();
  require(a.reservations.size() == b.reservations.size() &&
              !a.reservations.empty(),
          "entity-id perturbation changed booking count");
  for (std::size_t index = 0; index < a.reservations.size(); ++index)
    require(a.reservations[index].profile == b.reservations[index].profile,
            "unrelated entity-id churn changed guest profile stream");
}

void twenty_day_same_seed_campaign_is_byte_deterministic() {
  constexpr std::uint64_t seed = 0x484156454eULL;
  auto first = Simulation::tutorial(seed);
  auto second = Simulation::tutorial(seed);
  applyCampaignSetup(first);
  applyCampaignSetup(second);

  for (int day = 0; day < 10; ++day) {
    first.step(86400);
    second.step(86400);
    require(first.save() == second.save(),
            "same-seed guest campaign diverged before save checkpoint");
  }

  auto reloaded = Simulation::load(first.save());
  require(reloaded.save() == first.save(),
          "ten-day guest checkpoint was not byte-stable");

  for (int day = 10; day < 20; ++day) {
    first.step(86400);
    second.step(86400);
    reloaded.step(86400);
    require(first.save() == second.save(),
            "same-seed guest campaign diverged across twenty-day gate");
    require(first.save() == reloaded.save(),
            "loaded guest campaign diverged across twenty-day gate");
  }

  requireSameReviews(first.view(), second.view());
  requireSameReviews(first.view(), reloaded.view());
}
} // namespace

int main() {
  try {
    booking_profile_stream_ignores_unrelated_entity_ids();
    twenty_day_same_seed_campaign_is_byte_deterministic();
  } catch (const std::exception &error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
  std::cout << "Twenty-day guest determinism gate passed\n";
  return 0;
}
