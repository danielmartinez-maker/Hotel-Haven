#include "hh/game/MarketDemand.h"
#include <stdexcept>

using namespace hh::game;
static void require(bool value, const char *message) {
  if (!value) throw std::runtime_error(message);
}

int main() {
  MarketDemandSystem market(5150);
  market.setPlayerOffer({1, 16'000, 80, 4, 80, 80, 80, true});
  market.setPlayerConsiderationBasisPoints(MarketSegment::Business, 10000);

  CompetitorOffer rival{2, "Calendar Rival", 16'000, 80, 4, 80, 80, 80};
  rival.roomCount = 100;
  rival.roomCategories = {"standard", "suite"};
  rival.inventoryWindows = {
      {"standard", 20, 22, 18'000, 0},
      {"suite", 20, 22, 25'000, 5}};
  market.setCompetitors({rival});

  BookingRequest standard;
  standard.id = 7001;
  standard.segment = MarketSegment::Business;
  standard.arrivalDay = 21;
  standard.departureDay = 22;
  standard.budgetCents = 25'000;
  standard.roomCategory = "standard";
  const auto unavailable = market.effectiveCompetitorOffer(2, standard);
  require(unavailable.hotelId == 2 && unavailable.nightlyRateCents == 18'000 &&
              !unavailable.sellable,
          "dated standard competitor closure was not applied");

  auto suite = standard;
  suite.roomCategory = "suite";
  const auto suiteOffer = market.effectiveCompetitorOffer(2, suite);
  require(suiteOffer.nightlyRateCents == 25'000 && suiteOffer.sellable,
          "dated suite competitor rate/availability was not applied");

  auto unsupported = standard;
  unsupported.roomCategory = "family";
  require(!market.effectiveCompetitorOffer(2, unsupported).sellable,
          "competitor sold an unsupported room category");

  auto outsideWindow = standard;
  outsideWindow.arrivalDay = 30;
  outsideWindow.departureDay = 31;
  const auto baseline = market.effectiveCompetitorOffer(2, outsideWindow);
  require(baseline.sellable && baseline.nightlyRateCents == 16'000,
          "competitor baseline did not resume outside override window");

  auto profile = market.segmentDemandProfile(MarketSegment::Business);
  profile.baseDailyDemand = 50.0;
  profile.medianLeadTimeDays = 0;
  profile.medianStayNights = 1;
  profile.baseBudgetCents = 25'000;
  market.setSegmentDemandProfile(profile);
  require(market.generatePotentialRequests(MarketSegment::Business, 21,
                                            MarketDemandModifiers{}) == 50,
          "calendar allocation fixture demand generation changed");
  require(market.snapshot().competitorWins == 0 && market.snapshot().playerWins == 50,
          "unavailable competitor still captured dated demand");

  const auto saved = market.save();
  const auto restored = MarketDemandSystem::load(saved);
  require(restored.save() == saved,
          "competitor calendar did not round-trip");
  require(restored.effectiveCompetitorOffer(2, standard) == unavailable,
          "competitor calendar behavior changed after load");
}
