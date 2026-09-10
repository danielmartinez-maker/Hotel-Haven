#include "hh/game/EconomyRuntime.h"
#include <stdexcept>

using namespace hh::game;
static void require(bool value, const char *message) {
  if (!value) throw std::runtime_error(message);
}

int main() {
  EconomyRuntime baseline(12001, 5'000'000);
  EconomyRuntime marketed(12001, 5'000'000);

  MarketingCampaign daily;
  daily.id = 501;
  daily.startDay = 0;
  daily.endDay = 2;
  daily.visibilityBoostBasisPoints = 1000;
  daily.targetSegments = {MarketSegment::Business};
  daily.dailyCostCents = 1'000;
  require(marketed.startMarketingCampaign(daily).ok,
          "daily-cost marketing campaign rejected");
  require(marketed.financialSnapshot().economics.cashCents ==
              baseline.financialSnapshot().economics.cashCents,
          "daily campaign charged before its scheduled day");

  baseline.runDays(2);
  marketed.runDays(2);
  require(baseline.financialSnapshot().economics.cashCents -
              marketed.financialSnapshot().economics.cashCents == 2'000,
          "daily campaign did not charge once on each active day");

  const auto saved = marketed.save();
  auto restored = EconomyRuntime::load(saved);
  require(restored.save() == saved,
          "daily campaign billing state did not round-trip");

  baseline.runDays(2);
  marketed.runDays(2);
  restored.runDays(2);
  require(baseline.financialSnapshot().economics.cashCents -
              marketed.financialSnapshot().economics.cashCents == 3'000,
          "daily campaign billed outside its inclusive active window");
  require(restored.authoritativeHash() == marketed.authoritativeHash(),
          "save/load continuation duplicated or skipped daily campaign billing");

  MarketingCampaign ambiguous = daily;
  ambiguous.id = 502;
  ambiguous.costCents = 5'000;
  require(!marketed.startMarketingCampaign(ambiguous).ok,
          "campaign accepted both fixed and daily billing modes");
}
