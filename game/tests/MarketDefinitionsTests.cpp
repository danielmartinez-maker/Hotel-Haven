#include "hh/game/EconomyRuntime.h"
#include "hh/game/MarketDemand.h"
#include <cmath>
#include <stdexcept>

using namespace hh::game;
static void require(bool value, const char *message) {
  if (!value) throw std::runtime_error(message);
}

static constexpr const char *kDefinitions = R"JSON({
  "schema_version": 2,
  "choice_temperature_basis_points": 3500,
  "segments": [
    {
      "id": "business",
      "base_daily_demand": 12.5,
      "budget_cents": {"median": 23000},
      "price_elasticity_bp": 20000,
      "amenity_sensitivity_bp": 8000,
      "lead_time_days": {"median": 7},
      "stay_nights": {"median": 2},
      "cancellation_bp": 1200,
      "no_show_bp": 300,
      "weekday_multiplier_bp": [9000, 10000, 11000, 12000, 13000, 8000, 7000],
      "choice_weights_basis_points": {
        "price": 3000,
        "reputation": 2000,
        "amenity": 1500,
        "location": 1000,
        "star": 1000,
        "room": 500,
        "brand": 1000
      }
    }
  ]
})JSON";

int main() {
  MarketDemandSystem baseline(6060);
  MarketDemandSystem configured(6060);
  const auto loaded = configured.loadDefinitions(kDefinitions);
  require(loaded.ok, "valid market definitions were rejected");
  const auto &profile = configured.segmentDemandProfile(MarketSegment::Business);
  require(std::abs(profile.baseDailyDemand - 12.5) < 1e-9 &&
              profile.baseBudgetCents == 23'000 &&
              profile.medianLeadTimeDays == 7 && profile.medianStayNights == 2,
          "business demand definition was not applied");
  require(profile.priceElasticityBasisPoints == 20'000 &&
              profile.amenitySensitivityBasisPoints == 8'000 &&
              profile.cancellationBasisPoints == 1'200 &&
              profile.noShowBasisPoints == 300,
          "business behavioral definition was not applied");
  require(profile.weekdayMultiplierBasisPoints[3] == 12'000,
          "weekday profile was not loaded from data");
  require(configured.choiceTemperatureBasisPoints() == 3'500,
          "choice temperature was not loaded from data");

  BookingRequest request{1, MarketSegment::Business, 10, 11, 20'000, 1, 50, 50, 50, 0};
  MarketHotelOffer hotel{1, 20'000, 75, 4, 75, 75, 75, true};
  require(configured.playerChoiceWeight(request, hotel) <
              baseline.playerChoiceWeight(request, hotel),
          "configured price elasticity was not used by hotel choice");

  const auto beforeInvalid = configured.save();
  const auto invalid = configured.loadDefinitions(R"JSON({
    "schema_version": 2,
    "segments": [{
      "id": "business",
      "choice_weights_basis_points": {
        "price": 9000, "reputation": 9000, "amenity": 0,
        "location": 0, "star": 0, "room": 0, "brand": 0
      }
    }]
  })JSON");
  require(!invalid.ok && configured.save() == beforeInvalid,
          "invalid definitions partially mutated authoritative market state");

  EconomyRuntime runtime(6060);
  require(runtime.loadMarketDefinitions(kDefinitions).ok,
          "economy runtime did not expose market definition loading");
  require(runtime.marketChoiceTemperatureBasisPoints() == 3'500,
          "runtime did not expose configured market temperature");

  const auto saved = configured.save();
  require(MarketDemandSystem::load(saved).save() == saved,
          "data-driven market settings did not persist");
}
