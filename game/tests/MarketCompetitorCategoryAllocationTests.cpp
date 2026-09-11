#include "hh/game/MarketDemand.h"
#include <stdexcept>

using namespace hh::game;
static void require(bool value, const char *message) {
  if (!value) throw std::runtime_error(message);
}

static MarketDemandSystem fixture(int competitorCategoryScore) {
  MarketDemandSystem market(4404);
  MarketHotelOffer player{1, 16'000, 70, 4, 80, 80, 80, true};
  player.reputationCategories = {70, 70, 70, 70, 70, 70};
  market.setPlayerOffer(player);
  market.setPlayerConsiderationBasisPoints(MarketSegment::Business, 10000);

  CompetitorOffer rival{2, "Business Rival", 16'000, 70, 4, 80, 80, 80};
  rival.reputationCategories = {competitorCategoryScore, competitorCategoryScore,
                                competitorCategoryScore, competitorCategoryScore,
                                competitorCategoryScore, competitorCategoryScore};
  market.setCompetitors({rival});

  auto profile = market.segmentDemandProfile(MarketSegment::Business);
  profile.baseDailyDemand = 500.0;
  profile.medianLeadTimeDays = 0;
  profile.medianStayNights = 1;
  profile.baseBudgetCents = 20'000;
  market.setSegmentDemandProfile(profile);
  return market;
}

int main() {
  auto weakRival = fixture(20);
  auto strongRival = fixture(100);
  const MarketDemandModifiers baseline{};
  require(weakRival.generatePotentialRequests(MarketSegment::Business, 20, baseline) == 500,
          "weak-rival fixture demand generation changed");
  require(strongRival.generatePotentialRequests(MarketSegment::Business, 20, baseline) == 500,
          "strong-rival fixture demand generation changed");

  require(weakRival.snapshot().playerWins > strongRival.snapshot().playerWins,
          "competitor category reputation was ignored by aggregate allocation");
}
