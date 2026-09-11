#include "hh/game/MarketDemand.h"
#include <cmath>
#include <stdexcept>

using namespace hh::game;
static void require(bool value, const char *message) {
  if (!value) throw std::runtime_error(message);
}
static void requireNear(double a, double b, double epsilon, const char *message) {
  if (std::abs(a - b) > epsilon) throw std::runtime_error(message);
}

int main() {
  MarketDemandSystem market(303);
  BookingRequest business;
  business.id = 1;
  business.segment = MarketSegment::Business;
  business.arrivalDay = 20;
  business.departureDay = 22;
  business.budgetCents = 20'000;

  MarketHotelOffer categorical{1, 16'000, 80, 4, 80, 80, 80, true};
  categorical.reputationCategories = {100, 50, 50, 50, 50, 50};
  requireNear(market.reputationUtility(business, categorical), 0.625, 1e-12,
              "Business reputation weights diverged from HMG-030 example");

  MarketHotelOffer fallback{1, 16'000, 80, 4, 80, 80, 80, true};
  requireNear(market.reputationUtility(business, fallback), 0.80, 1e-12,
              "missing category data did not fall back to overall reputation");

  auto stronger = categorical;
  stronger.reputationCategories = {100, 100, 100, 100, 100, 100};
  require(market.playerChoiceWeight(business, stronger) >
              market.playerChoiceWeight(business, categorical),
          "improved Business reputation categories did not increase choice weight");

  market.setPlayerOffer(categorical);
  CompetitorOffer competitor{2, "Categorical Rival", 16'500, 78, 4, 80, 80, 80};
  competitor.reputationCategories = {70, 75, 80, 65, 85, 70};
  market.setCompetitors({competitor});
  const auto saved = market.save();
  const auto restored = MarketDemandSystem::load(saved);
  require(restored.save() == saved,
          "market reputation categories did not round-trip");
  require(restored.snapshot().competitors.front().reputationCategories ==
              competitor.reputationCategories,
          "competitor reputation categories were lost on save/load");
}
