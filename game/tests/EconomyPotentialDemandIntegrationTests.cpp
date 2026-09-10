#include "hh/game/EconomyRuntime.h"
#include <array>
#include <stdexcept>

using namespace hh::game;
static void require(bool v, const char *m) { if (!v) throw std::runtime_error(m); }

int main() {
  EconomyRuntime baseline(3031), boosted(3031);
  for (auto *runtime : {&baseline, &boosted}) {
    runtime->setPhysicalRoomCapacity("standard", 40);
    runtime->setPlayerHotelOffer({1, 17'000, 76, 4, 80, 80, 80, true});
    runtime->setCompetitors({{2, "Rival", 17'000, 75, 4, 80, 80, 80}});
  }

  MarketDemandModifiers modifiers;
  modifiers.scenarioMultiplierBasisPoints = 20'000;
  boosted.setDemandModifiers(modifiers);

  baseline.runDays(1);
  boosted.runDays(1);

  const auto baseMarket = baseline.marketSnapshot();
  const auto boostedMarket = boosted.marketSnapshot();
  require(baseMarket.generatedRequests == 119,
          "default HMG-030 segment demand did not generate 119 requests/day");
  require(boostedMarket.generatedRequests == 238,
          "scenario demand multiplier did not scale potential requests deterministically");

  std::array<bool, 9> seen{};
  for (const auto &request : baseMarket.requests) {
    const int segment = static_cast<int>(request.segment);
    require(segment >= 0 && segment < 9, "generated invalid market segment");
    seen[static_cast<std::size_t>(segment)] = true;
    require(request.bookingDay == 0,
            "runtime generated request on a day other than its lead-time booking day");
  }
  for (const bool present : seen)
    require(present, "runtime omitted an HMG-030 market segment");

  const auto saved = boosted.save();
  auto restored = EconomyRuntime::load(saved);
  require(restored.save() == saved,
          "campaign demand modifiers did not round-trip in economy runtime state");
  boosted.runDays(1);
  restored.runDays(1);
  require(boosted.authoritativeHash() == restored.authoritativeHash(),
          "modifier-driven market continuation diverged after save/load");
}
