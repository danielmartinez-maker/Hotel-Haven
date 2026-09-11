#include "hh/game/EconomyRuntime.h"
#include <stdexcept>

using namespace hh::game;
static void require(bool v, const char *m) { if (!v) throw std::runtime_error(m); }

int main() {
  EconomyRuntime a(606);
  a.setPhysicalRoomCapacity("standard", 24);
  a.setPlayerHotelOffer({1, 15000, 78, 4, 82, 80, 75, true});
  a.setCompetitors({{2, "Comparable", 15000, 70, 4, 78, 78, 72},
                    {3, "Premium", 21000, 86, 5, 90, 88, 86}});
  require(a.setPricingRule({1, 0, 365, 0x7f, "standard", 0, 10000, 15000, 1, 8000, 30000}).ok,
          "pricing command rejected");
  require(a.setOverbookingPolicy({"standard", 2, 30000}).ok,
          "overbooking command rejected");
  a.runDays(180);
  auto market = a.marketSnapshot();
  auto finance = a.financialSnapshot();
  auto revenue = a.revenueManagementSnapshot();
  require(market.generatedRequests > 0, "180-day campaign generated no market demand");
  require(revenue.inventory.minimumAvailableUnits >= 0,
          "future inventory fell negative beyond configured overbooking allowance");
  require(finance.economics.transactionCount > 0 && finance.economics.reconciled,
          "economic transactions did not reconcile");

  const auto saved = a.save();
  auto restored = EconomyRuntime::load(saved);
  require(restored.save() == saved, "FINAL-06 runtime did not round-trip exactly");
  a.runDays(30); restored.runDays(30);
  require(restored.authoritativeHash() == a.authoritativeHash(),
          "save/load continuation diverged authoritative FINAL-06 state");

  EconomyRuntime stronger(99), weaker(99);
  stronger.setPhysicalRoomCapacity("standard", 20);
  weaker.setPhysicalRoomCapacity("standard", 20);
  stronger.setPlayerHotelOffer({1, 16000, 90, 4, 80, 80, 80, true});
  weaker.setPlayerHotelOffer({1, 16000, 60, 4, 80, 80, 80, true});
  const std::vector<CompetitorOffer> comps{{2, "Rival", 16000, 75, 4, 80, 80, 80}};
  stronger.setCompetitors(comps); weaker.setCompetitors(comps);
  stronger.runDays(60); weaker.runDays(60);
  require(stronger.marketSnapshot().playerWins >= weaker.marketSnapshot().playerWins,
          "stronger reputation lost demand at comparable rate");

  EconomyRuntime priced(100), overpriced(100);
  priced.setPhysicalRoomCapacity("standard", 20);
  overpriced.setPhysicalRoomCapacity("standard", 20);
  priced.setPlayerHotelOffer({1, 14000, 75, 4, 80, 80, 80, true});
  overpriced.setPlayerHotelOffer({1, 26000, 75, 4, 80, 80, 80, true});
  priced.setCompetitors(comps); overpriced.setCompetitors(comps);
  priced.runDays(60); overpriced.runDays(60);
  require(overpriced.marketSnapshot().playerWins < priced.marketSnapshot().playerWins,
          "material overpricing did not reduce price-sensitive demand");
}
