#include "Final06UiCommandAdapter.h"
#include "GameUiBridge.h"
#include "hh/frontend/EconomyDashboard.h"
#include "hh/game/SimulationEconomyBridge.h"

#include <stdexcept>
#include <string>

namespace {
void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}
} // namespace

int main() {
  auto authority = hh::game::SimulationEconomyBridge::tutorial(20260910);
  authority.setPlayerHotelOffer({1, 15000, 75, 4, 80, 80, 75, true});
  authority.setCompetitors({
      {2, "Comparable", 16000, 76, 4, 80, 80, 75},
      {3, "Premium", 22000, 88, 5, 92, 90, 88},
  });
  require(authority.setOverbookingPolicy({"standard", 0, 25000, 2, 20}).ok,
          "test fixture could not install authoritative overbooking policy");

  const auto beforeSnapshot = authority.save();
  hh::client::GameUiBridgeContext context;
  context.simulationSpeed = 0;
  context.activeFloor = 0;
  context.cutaway = true;
  context.currentTool = "Inspect";
  const auto source = hh::client::makeGameUiSnapshotSource(authority, context);

  require(source.economy.kpis.cashCents ==
              authority.financialSnapshot().economics.cashCents,
          "finance cash did not bind to integrated FINAL-06 authority");
  require(source.economy.competitors.size() == 2,
          "competitor comparison was not exposed by the UI snapshot");
  require(authority.save() == beforeSnapshot,
          "building the FINAL-07 snapshot mutated integrated authority");

  hh::frontend::EconomyDashboard dashboard;
  const auto rate = dashboard.setFutureRateCommand(4, "standard", 17500);
  const auto rateResult = hh::client::dispatchFinal06UiCommand(authority, rate);
  require(rateResult.ok, "future-rate command did not reach FINAL-06 authority");
  const auto revenue = authority.revenueManagementSnapshot();
  require(revenue.rules.size() == 1 && revenue.rules.front().startDay == 4 &&
              revenue.rules.front().endDay == 4 &&
              revenue.rules.front().roomCategory == "standard" &&
              revenue.rules.front().rateCents == 17500,
          "future-rate command fields changed at the application seam");

  const auto overbooking = dashboard.setOverbookingCommand(1);
  const auto overbookingResult =
      hh::client::dispatchFinal06UiCommand(authority, overbooking);
  require(overbookingResult.ok,
          "overbooking command did not reach FINAL-06 authority");
  const auto policies = authority.overbookingPolicies();
  const auto standardPolicy = policies.find("standard");
  require(standardPolicy != policies.end() && standardPolicy->second.allowance == 1,
          "overbooking UI command did not update the requested allowance");
  require(standardPolicy->second.relocationCompensationCents == 25000 &&
              standardPolicy->second.startDay == 2 &&
              standardPolicy->second.endDay == 20,
          "overbooking UI command changed authority-owned recovery policy fields");

  const auto campaign = dashboard.startCampaignCommand("1");
  const auto campaignResult =
      hh::client::dispatchFinal06UiCommand(authority, campaign);
  require(!campaignResult.ok &&
              campaignResult.reasonCode == "FINAL06_CAMPAIGN_OFFER_UNAVAILABLE",
          "FINAL-07 fabricated a campaign definition that authority did not expose");

  const auto contract = dashboard.acceptContractCommand("1");
  const auto contractResult =
      hh::client::dispatchFinal06UiCommand(authority, contract);
  require(!contractResult.ok &&
              contractResult.reasonCode == "FINAL06_CONTRACT_OFFER_UNAVAILABLE",
          "FINAL-07 fabricated a contract definition that authority did not expose");
}
