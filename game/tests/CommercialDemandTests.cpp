#include "hh/game/CommercialDemand.h"
#include <stdexcept>

using namespace hh::game;
static void require(bool v, const char *m) { if (!v) throw std::runtime_error(m); }

int main() {
  CommercialDemand commercial;
  commercial.setReputation(6000);
  commercial.applyReview({1, 9000, 7000, 8500, 8000});
  require(commercial.snapshot().overallReputationBasisPoints > 6000,
          "positive review did not improve decayed/EWMA reputation");

  MarketingCampaign campaign{11, 10, 20, 150000, 1500, {MarketSegment::Leisure}};
  require(commercial.startCampaign(campaign, 10).ok, "valid campaign rejected");
  require(commercial.visibilityBasisPoints(MarketSegment::Leisure, 12) > 10000,
          "marketing did not increase considered-set visibility");
  require(commercial.visibilityBasisPoints(MarketSegment::Business, 12) == 10000,
          "marketing changed an untargeted segment");

  CommercialContract contract;
  contract.id = 22;
  contract.startDay = 30;
  contract.endDay = 32;
  contract.minimumRoomNights = 20;
  contract.maximumRoomNights = 30;
  contract.negotiatedRateCents = 11000;
  contract.requiredVenueCapacity = 100;
  contract.requiredServiceUnits = 5;
  ContractFeasibility impossible{15, 80, 4};
  require(!commercial.acceptContract(contract, impossible, false).ok,
          "infeasible contract silently accepted without risk route");
  const auto risky = commercial.acceptContract(contract, impossible, true);
  require(risky.ok && risky.acceptedRisk,
          "explicit accept-risk route did not preserve infeasible contract risk");
}
