#include "hh/game/SimulationEconomyBridge.h"
#include <stdexcept>

using namespace hh::game;
static void require(bool v, const char *m) { if (!v) throw std::runtime_error(m); }

int main() {
  auto bridge = SimulationEconomyBridge::tutorial(9001);
  const auto initialView = bridge.view();
  const auto initialFinancial = bridge.financialSnapshot();
  require(initialFinancial.economics.cashCents == initialView.economy.cashCents,
          "bridge ledger did not start from authoritative simulation cash");
  require(initialFinancial.economics.reconciled,
          "bridge ledger did not start reconciled");

  const auto beforeCapital = initialFinancial.economics.capitalExpenseCents;
  const auto beforeCash = initialFinancial.economics.cashCents;
  const auto built = bridge.buildTile({0, 31, 19}, TileKind::Floor);
  require(built.ok, "bridge construction command failed");
  const auto afterBuild = bridge.financialSnapshot();
  require(afterBuild.economics.cashCents == beforeCash - 500,
          "construction cash delta was not mirrored exactly once");
  require(afterBuild.economics.capitalExpenseCents == beforeCapital + 500,
          "construction did not post to capital expense");
  require(afterBuild.economics.cashCents == bridge.view().economy.cashCents,
          "bridge ledger diverged from simulation cash after construction");

  const auto suppliesBefore = bridge.financialSnapshot().economics.operatingCostCents;
  const auto supply = bridge.orderSupplies({1, 0, 0, 0, 0});
  require(supply.ok, "bridge supply order failed");
  const auto suppliesAfter = bridge.financialSnapshot();
  require(suppliesAfter.economics.operatingCostCents == suppliesBefore + 1200,
          "supply purchase did not post to operating cost");
  require(suppliesAfter.economics.cashCents == bridge.view().economy.cashCents,
          "bridge ledger diverged from simulation cash after supplies");

  bridge.setPlayerHotelOffer({1, 15'000, 75, 4, 80, 80, 75, true});
  bridge.setCompetitors({{2, "Comparable", 16'000, 76, 4, 80, 80, 75},
                         {3, "Premium", 22'000, 88, 5, 92, 90, 88}});
  require(bridge.marketSnapshot().comparableMedianRateCents == 19'000,
          "bridge did not expose FINAL-06 market state");
  require(bridge.setOverbookingPolicy({"standard", 1, 25'000, 5, 10}).ok,
          "bridge rejected valid dated overbooking policy");

  const auto saved = bridge.save();
  auto restored = SimulationEconomyBridge::load(saved);
  require(restored.save() == saved,
          "integrated Simulation/FINAL-06 bridge did not round-trip exactly");
  require(restored.authoritativeHash() == bridge.authoritativeHash(),
          "integrated bridge authoritative hash changed on load");
  require(restored.financialSnapshot().economics.cashCents ==
              restored.view().economy.cashCents,
          "restored bridge ledger diverged from simulation cash");
}
