#include "hh/game/CommercialDemand.h"
#include "hh/game/RevenueInventory.h"
#include <stdexcept>

using namespace hh::game;
static void require(bool v, const char *m) { if (!v) throw std::runtime_error(m); }

int main() {
  RevenueInventory inventory(77);
  inventory.setPhysicalCapacity("standard", 1);
  inventory.setOverbookingAllowance("standard", 1, 10, 12);
  require(inventory.sellableUnits(9, "standard") == 1,
          "dated overbooking allowance leaked before start day");
  require(inventory.sellableUnits(10, "standard") == 2 &&
              inventory.sellableUnits(12, "standard") == 2,
          "dated overbooking allowance missing inside window");
  require(inventory.sellableUnits(13, "standard") == 1,
          "dated overbooking allowance leaked after end day");
  require(inventory.book({1, 10, 11, "standard", 15'000, BookingChannel::Direct}).ok,
          "first in-window booking failed");
  require(inventory.book({2, 10, 11, "standard", 15'000, BookingChannel::Ota}).ok,
          "allowed in-window overbooking failed");
  require(!inventory.book({3, 10, 11, "standard", 15'000, BookingChannel::Direct}).ok,
          "inventory sold beyond dated overbooking allowance");
  require(inventory.book({4, 13, 14, "standard", 15'000, BookingChannel::Direct}).ok,
          "first post-window booking failed");
  require(!inventory.book({5, 13, 14, "standard", 15'000, BookingChannel::Direct}).ok,
          "post-window inventory incorrectly retained overbooking allowance");
  const auto inventorySave = inventory.save();
  require(RevenueInventory::load(inventorySave).save() == inventorySave,
          "dated inventory allowance did not round-trip");

  CommercialDemand lowVolume;
  CommercialDemand highVolume;
  lowVolume.setReputation(7000);
  highVolume.setReputation(7000);
  for (std::uint64_t id = 1; id <= 100; ++id)
    highVolume.applyReview({id, 7000, 7000, 7000, 7000});
  lowVolume.applyReview({1001, 10000, 10000, 10000, 10000});
  highVolume.applyReview({1001, 10000, 10000, 10000, 10000});
  const int lowDelta = lowVolume.snapshot().overallReputationBasisPoints - 7000;
  const int highDelta = highVolume.snapshot().overallReputationBasisPoints - 7000;
  require(lowDelta > highDelta && highDelta > 0,
          "review-volume EWMA did not move low-volume reputation faster");
  require(lowVolume.snapshot().reviewCount == 1 &&
              highVolume.snapshot().reviewCount == 101,
          "reputation review volume was not retained");

  MarketingCampaign wellness;
  wellness.id = 42;
  wellness.startDay = 10;
  wellness.endDay = 20;
  wellness.costCents = 50'000;
  wellness.visibilityBoostBasisPoints = 1500;
  wellness.targetSegments = {MarketSegment::Wellness, MarketSegment::ExecutiveBusiness};
  require(highVolume.startCampaign(wellness, 0).ok,
          "campaign targeting expanded HMG-030 segments was rejected");
  const auto commercialSave = highVolume.save();
  const auto restored = CommercialDemand::load(commercialSave);
  require(restored.save() == commercialSave,
          "expanded-segment commercial state did not round-trip");
  require(restored.snapshot().reviewCount == highVolume.snapshot().reviewCount,
          "commercial review volume was lost on save/load");
}
