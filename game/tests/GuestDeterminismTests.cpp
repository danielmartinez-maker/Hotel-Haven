#include "hh/game/Simulation.h"
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

  const auto firstView = first.view();
  const auto secondView = second.view();
  require(firstView.reviews == secondView.reviews,
          "same-seed campaign produced different review history");
  require(firstView.reservations == secondView.reservations,
          "same-seed campaign produced different guest decisions/history");
}
} // namespace

int main() {
  try {
    twenty_day_same_seed_campaign_is_byte_deterministic();
  } catch (const std::exception &error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
  std::cout << "Twenty-day guest determinism gate passed\n";
  return 0;
}
