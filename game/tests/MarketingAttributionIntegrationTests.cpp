#include "hh/game/CommercialDemand.h"
#include "hh/game/EconomyRuntime.h"
#include <stdexcept>

using namespace hh::game;
static void require(bool v, const char *m) { if (!v) throw std::runtime_error(m); }

int main() {
  CommercialDemand commercial;
  MarketingCampaign campaign;
  campaign.id = 50;
  campaign.startDay = 10;
  campaign.endDay = 12;
  campaign.costCents = 100'000;
  campaign.visibilityBoostBasisPoints = 4000;
  campaign.targetSegments = {MarketSegment::Business};
  campaign.rampUpDays = 2;
  campaign.attributionDecayDays = 2;
  require(commercial.startCampaign(campaign, 0).ok,
          "scheduled marketing campaign rejected");
  require(commercial.visibilityBasisPoints(MarketSegment::Business, 9) == 10000,
          "campaign visibility leaked before start");
  const int ramp = commercial.visibilityBasisPoints(MarketSegment::Business, 10);
  const int full = commercial.visibilityBasisPoints(MarketSegment::Business, 11);
  const int decayed = commercial.visibilityBasisPoints(MarketSegment::Business, 13);
  require(ramp > 10000 && ramp < full,
          "campaign ramp-up did not progressively increase visibility");
  require(full == 14000,
          "campaign did not reach configured visibility boost");
  require(decayed > 10000 && decayed < full,
          "campaign attribution did not decay after active window");
  require(commercial.visibilityBasisPoints(MarketSegment::Business, 14) == 10000,
          "campaign attribution did not expire after configured decay");

  EconomyRuntime baseline(707), marketed(707);
  for (auto *runtime : {&baseline, &marketed}) {
    runtime->setPhysicalRoomCapacity("standard", 40);
    runtime->setPlayerHotelOffer({1, 17'000, 75, 4, 80, 80, 80, true});
    runtime->setCompetitors({{2, "Equal Rival", 17'000, 75, 4, 80, 80, 80}});
  }
  MarketingCampaign broad;
  broad.id = 51;
  broad.startDay = 0;
  broad.endDay = 0;
  broad.visibilityBoostBasisPoints = 10'000;
  broad.targetSegments = {MarketSegment::BudgetLeisure, MarketSegment::Business,
                          MarketSegment::ExecutiveBusiness, MarketSegment::CoupleLeisure,
                          MarketSegment::FamilyLeisure, MarketSegment::LuxuryLeisure,
                          MarketSegment::ConferenceGroup, MarketSegment::AirportTransit,
                          MarketSegment::Wellness};
  require(marketed.startMarketingCampaign(broad).ok,
          "runtime marketing command rejected");

  baseline.runDays(1);
  marketed.runDays(1);
  const auto baseMarket = baseline.marketSnapshot();
  const auto marketedMarket = marketed.marketSnapshot();
  require(marketedMarket.generatedRequests == baseMarket.generatedRequests,
          "marketing incorrectly changed potential traveler volume");
  require(marketedMarket.playerWins > baseMarket.playerWins,
          "marketing visibility did not increase player considered-set wins");
  require(marketedMarket.competitorWins > 0,
          "marketing incorrectly guaranteed every booking");

  const auto saved = marketed.save();
  auto restored = EconomyRuntime::load(saved);
  require(restored.save() == saved,
          "marketing attribution state did not round-trip");
}
