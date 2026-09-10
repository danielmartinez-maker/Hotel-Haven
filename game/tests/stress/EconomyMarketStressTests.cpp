#include "StressHarness.h"
#include "hh/game/CommercialDemand.h"
#include "hh/game/EconomyRuntime.h"
#include "hh/game/Financing.h"
#include "hh/game/MarketDemand.h"
#include "hh/game/Overbooking.h"
#include "hh/game/RevenueInventory.h"
#include "hh/game/RevenueManagement.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace {
using namespace hh::game;

struct ScaleBudget {
  int soakDays;
  std::size_t operations;
  int roomCapacity;
};

ScaleBudget budget(hh::stress::Scale scale) {
  switch (scale) {
  case hh::stress::Scale::Pr:
    return {180, 50'000, 6'000};
  case hh::stress::Scale::Extended:
    return {2'000, 500'000, 600};
  case hh::stress::Scale::Exhaustive:
    return {3'650, 2'000'000, 300};
  }
  std::abort();
}

void assertInventory(const RevenueInventorySnapshot &inventory,
                     hh::stress::RunContext &ctx,
                     std::uint64_t checkpoint) {
  std::unordered_set<std::uint64_t> ids;
  if (inventory.minimumAvailableUnits < 0 ||
      inventory.hotPathBookingCount > inventory.bookings.size() ||
      inventory.bookedRoomRevenueCents < 0 ||
      inventory.channelCommissionCents < 0)
    ctx.fail("revenue inventory aggregate invariant violated", checkpoint);
  for (const auto &booking : inventory.bookings) {
    if (booking.bookingId == 0 || !ids.insert(booking.bookingId).second ||
        booking.arrivalDay < 0 || booking.departureDay <= booking.arrivalDay ||
        booking.roomCategory.empty() || booking.rateCents < 0 ||
        booking.commissionBasisPoints < 0 || booking.commissionCents < 0)
      ctx.fail("booking state invariant violated", checkpoint);
    const int state = static_cast<int>(booking.state);
    if (state < static_cast<int>(BookingState::Confirmed) ||
        state > static_cast<int>(BookingState::Completed))
      ctx.fail("illegal booking state", checkpoint);
  }
}

void assertRuntime(const EconomyRuntime &runtime,
                   hh::stress::RunContext &ctx,
                   std::uint64_t checkpoint) {
  const auto market = runtime.marketSnapshot();
  if (market.generatedRequests !=
          market.playerWins + market.competitorWins + market.unallocatedRequests ||
      market.requests.size() != market.generatedRequests ||
      market.choices.size() != market.generatedRequests ||
      market.comparableMedianRateCents < 0 || market.physicalCompetitorGuests < 0)
    ctx.fail("market allocation conservation violated", checkpoint,
             std::to_string(runtime.authoritativeHash()));

  const auto revenue = runtime.revenueManagementSnapshot();
  assertInventory(revenue.inventory, ctx, checkpoint);
  for (const auto &[category, rate] : revenue.effectiveRateCents)
    if (category.empty() || rate < 0)
      ctx.fail("invalid effective room rate", checkpoint);

  const auto financial = runtime.financialSnapshot();
  const auto &economics = financial.economics;
  if (!economics.reconciled || !std::isfinite(economics.occupancy) ||
      economics.occupancy < 0.0 || economics.occupancy > 1.0 ||
      economics.totalRevenueCents < 0 || economics.roomRevenueCents < 0 ||
      economics.nonRoomRevenueCents < 0 || economics.operatingCostCents < 0 ||
      economics.debtServiceCents < 0 || economics.capitalExpenseCents < 0 ||
      economics.adrCents < 0 || economics.revParCents < 0 ||
      economics.tRevParCents < 0)
    ctx.fail("exact-cent economics/reconciliation invariant violated", checkpoint,
             std::to_string(runtime.authoritativeHash()));

  const auto &financing = financial.financing;
  if (financing.outstandingPrincipalCents < 0 ||
      financing.nextDebtServiceCents < 0 || financing.missedObligationCents < 0)
    ctx.fail("financing balance invariant violated", checkpoint);

  const auto commercial = runtime.commercialSnapshot();
  const auto bounded = [](int value) { return value >= 0 && value <= 10'000; };
  if (!bounded(commercial.overallReputationBasisPoints) ||
      !bounded(commercial.serviceReputationBasisPoints) ||
      !bounded(commercial.cleanlinessReputationBasisPoints) ||
      !bounded(commercial.valueReputationBasisPoints))
    ctx.fail("commercial reputation escaped basis-point bounds", checkpoint);

  if (runtime.estimatedStateBytes() > 64ULL * 1024ULL * 1024ULL)
    ctx.fail("economy state exceeded supported save envelope", checkpoint,
             std::to_string(runtime.estimatedStateBytes()));
}

void configureRuntime(EconomyRuntime &runtime, const ScaleBudget &limits) {
  runtime.setPhysicalRoomCapacity("standard", limits.roomCapacity);
  runtime.setPlayerHotelOffer({1, 15'000, 80, 4, 82, 80, 78, true});
  runtime.setCompetitors({{2, "Comparable", 15'500, 76, 4, 80, 79, 74},
                          {3, "Premium", 22'000, 88, 5, 92, 90, 87},
                          {4, "Value", 11'000, 70, 3, 68, 75, 65}});
  if (!runtime.setPricingRule(
           {1, 0, 20'000, 0x7f, "standard", 0, 10000, 15'000, 1,
            8'000, 35'000})
           .ok)
    throw std::runtime_error("runtime base pricing rule rejected");
  if (!runtime.setPricingRule(
           {2, 0, 20'000, 0x7f, "standard", 8500, 10000, 21'000, 10,
            8'000, 35'000})
           .ok)
    throw std::runtime_error("runtime high-occupancy pricing rule rejected");
  if (!runtime.setOverbookingPolicy({"standard", 3, 30'000}).ok)
    throw std::runtime_error("runtime overbooking policy rejected");
}

void demandSpike(const hh::stress::Config &config, std::size_t operations) {
  hh::stress::RunContext ctx{"economy-market", config};
  ctx.phase = "demand_spike";
  MarketDemandSystem market(config.seed);
  market.setPlayerOffer({1, 14'000, 80, 4, 85, 80, 80, true});
  market.setCompetitors({{2, "Rival A", 15'000, 75, 4, 80, 80, 75},
                         {3, "Rival B", 20'000, 85, 5, 90, 85, 85}});
  market.generateRequests(1, 30, static_cast<int>(std::min<std::size_t>(operations, 2'000'000)));
  const auto snapshot = market.snapshot();
  if (snapshot.generatedRequests != operations ||
      snapshot.generatedRequests != snapshot.playerWins + snapshot.competitorWins +
                                        snapshot.unallocatedRequests ||
      snapshot.requests.size() != operations || snapshot.choices.size() != operations)
    ctx.fail("demand spike request/allocation conservation failed", operations);
  const auto saved = market.save();
  if (MarketDemandSystem::load(saved).save() != saved)
    ctx.fail("market demand save/load round-trip diverged", operations);
}

void futureBookSaturation(const hh::stress::Config &config,
                          std::size_t operations) {
  hh::stress::RunContext ctx{"economy-market", config};
  ctx.phase = "future_book_saturation";
  RevenueInventory inventory(config.seed ^ 0x123456789ULL);
  inventory.setPhysicalCapacity("standard", 100);
  inventory.setOverbookingAllowance("standard", 5);

  std::uint64_t nextId = 1;
  std::size_t attempts = 0;
  int window = 0;
  while (attempts < operations) {
    const int arrival = 10 + window * 3;
    for (int unit = 0; unit < 105 && attempts < operations; ++unit, ++attempts) {
      BookingRequestInput request{nextId++, arrival, arrival + 2, "standard",
                                  15'000, BookingChannel::Direct};
      if (!inventory.book(request).ok)
        ctx.fail("booking within explicit sellable allowance rejected", attempts);
    }
    BookingRequestInput overflow{nextId++, arrival, arrival + 2, "standard",
                                 15'000, BookingChannel::Direct};
    if (inventory.book(overflow).ok)
      ctx.fail("future inventory exceeded physical plus overbooking allowance", attempts);
    ++window;
  }
  assertInventory(inventory.snapshot(), ctx, attempts);
  const auto saved = inventory.save();
  if (RevenueInventory::load(saved).save() != saved)
    ctx.fail("future inventory save/load round-trip diverged", attempts);
}

void cancellationNoShowStorm(const hh::stress::Config &config) {
  hh::stress::RunContext ctx{"economy-market", config};
  ctx.phase = "cancellation_no_show_storm";
  RevenueInventory a(config.seed), b(config.seed);
  for (auto *inventory : {&a, &b}) {
    inventory->setPhysicalCapacity("standard", 10'000);
    inventory->setCancellationBasisPoints(BookingChannel::Ota, 5000);
    inventory->setNoShowBasisPoints(BookingChannel::Ota, 5000);
    for (std::uint64_t id = 1; id <= 5'000; ++id) {
      BookingRequestInput request{id, 30, 32, "standard", 12'000,
                                  BookingChannel::Ota};
      if (!inventory->book(request).ok)
        ctx.fail("cancellation storm fixture booking rejected", id);
    }
    inventory->processDay(29);
    inventory->processDay(30);
  }
  if (!(a.snapshot() == b.snapshot()))
    ctx.fail("cancellation/no-show processing diverged for identical seed", 30);
  std::size_t changed = 0;
  for (const auto &booking : a.snapshot().bookings)
    changed += booking.state != BookingState::Confirmed;
  if (changed == 0)
    ctx.fail("cancellation/no-show storm produced no state changes", 30);
}

void competitorRateShock(const hh::stress::Config &config) {
  hh::stress::RunContext ctx{"economy-market", config};
  ctx.phase = "competitor_rate_shock";
  MarketDemandSystem market(config.seed);
  market.setPlayerOffer({1, 16'000, 78, 4, 80, 80, 80, true});
  market.setCompetitors({{2, "Rival", 16'000, 78, 4, 80, 80, 80}});
  const auto before = market.snapshot().comparableMedianRateCents;
  market.setCompetitors({{2, "Rival", 8'000, 78, 4, 80, 80, 80},
                         {3, "Rival 2", 9'000, 78, 4, 80, 80, 80}});
  const auto after = market.snapshot().comparableMedianRateCents;
  if (before <= 0 || after <= 0 || after >= before)
    ctx.fail("competitor rate shock did not move comparable median", 0);
}

void pricingRuleOverlap(const hh::stress::Config &config,
                        std::size_t operations) {
  hh::stress::RunContext ctx{"economy-market", config};
  ctx.phase = "pricing_rule_overlap";
  RevenueManagement a, b;
  for (std::uint64_t id = 1; id <= 64; ++id) {
    PricingRuleCommand rule{id, 0, 10'000, 0x7f, "standard", 0, 10000,
                            10'000 + static_cast<std::int64_t>(id) * 100,
                            static_cast<int>(id), 8'000, 30'000};
    if (!a.setRule(rule).ok || !b.setRule(rule).ok)
      ctx.fail("overlapping pricing rule rejected", id);
  }
  for (std::size_t i = 0; i < operations; ++i) {
    const int occupancy = static_cast<int>(i % 10'001);
    const auto rateA = a.effectiveRateCents(static_cast<int>(i % 365),
                                            static_cast<int>(i % 7), "standard",
                                            occupancy, 15'000);
    const auto rateB = b.effectiveRateCents(static_cast<int>(i % 365),
                                            static_cast<int>(i % 7), "standard",
                                            occupancy, 15'000);
    if (rateA != rateB || rateA < 8'000 || rateA > 30'000)
      ctx.fail("pricing rule priority replay diverged", i);
  }
  if (a.save() != b.save())
    ctx.fail("pricing rule authoritative state diverged", operations);
}

void overbookingRecovery(const hh::stress::Config &config) {
  hh::stress::RunContext ctx{"economy-market", config};
  ctx.phase = "overbooking_recovery";
  OverbookingSystem system;
  if (!system.setPolicy({"standard", 2, 30'000}).ok)
    ctx.fail("overbooking policy rejected", 0);
  RecoveryContext equivalent{"standard", true, true, true, true, true, true};
  if (system.chooseRecovery(equivalent).action != RecoveryAction::CategoryEquivalentRoom)
    ctx.fail("recovery order skipped category-equivalent room", 1);
  RecoveryContext upgrade{"standard", false, true, true, true, true, true};
  if (system.chooseRecovery(upgrade).action != RecoveryAction::FreeUpgrade)
    ctx.fail("recovery order skipped free upgrade", 2);
  RecoveryContext relocate{"standard", false, false, false, false, false, true};
  const auto decision = system.chooseRecovery(relocate);
  if (decision.action != RecoveryAction::CompetitorRelocation ||
      decision.compensationCents != 30'000)
    ctx.fail("competitor relocation/compensation semantics changed", 3);
  if (OverbookingSystem::load(system.save()).save() != system.save())
    ctx.fail("overbooking policy save/load diverged", 4);
}

void marketingReputationFeedback(const hh::stress::Config &config) {
  hh::stress::RunContext ctx{"economy-market", config};
  ctx.phase = "marketing_reputation_feedback";
  CommercialDemand commercial;
  commercial.setReputation(6'000);
  for (std::uint64_t id = 1; id <= 10'000; ++id) {
    const int value = 4'000 + static_cast<int>(id % 6'001);
    commercial.applyReview({id, value, value, value, value});
  }
  const auto reputation = commercial.snapshot();
  const auto bounded = [](int value) { return value >= 0 && value <= 10'000; };
  if (!bounded(reputation.overallReputationBasisPoints) ||
      !bounded(reputation.serviceReputationBasisPoints) ||
      !bounded(reputation.cleanlinessReputationBasisPoints) ||
      !bounded(reputation.valueReputationBasisPoints))
    ctx.fail("reputation feedback escaped basis-point bounds", 10'000);
  MarketingCampaign campaign{1, 10, 30, 150'000, 1'500,
                             {MarketSegment::Leisure, MarketSegment::Business}};
  if (!commercial.startCampaign(campaign, 10).ok ||
      commercial.visibilityBasisPoints(MarketSegment::Leisure, 15) <= 10'000)
    ctx.fail("marketing campaign failed to increase targeted visibility", 15);
}

void corporateGroupPressure(const hh::stress::Config &config) {
  hh::stress::RunContext ctx{"economy-market", config};
  ctx.phase = "corporate_group_pressure";
  CommercialDemand commercial;
  for (std::uint64_t id = 1; id <= 2'000; ++id) {
    CommercialContract contract;
    contract.id = id;
    contract.startDay = 30 + static_cast<int>(id % 30);
    contract.endDay = contract.startDay + 2;
    contract.minimumRoomNights = 10;
    contract.maximumRoomNights = 20;
    contract.negotiatedRateCents = 11'000;
    contract.requiredVenueCapacity = 50;
    contract.requiredServiceUnits = 5;
    const bool risky = (id & 1ULL) != 0;
    ContractFeasibility feasibility = risky ? ContractFeasibility{5, 25, 2}
                                             : ContractFeasibility{20, 100, 10};
    const auto result = commercial.acceptContract(contract, feasibility, risky);
    if (!result.ok || result.acceptedRisk != risky)
      ctx.fail("commercial contract risk semantics changed", id);
  }
  if (commercial.snapshot().contracts.size() != 2'000)
    ctx.fail("accepted commercial contracts were lost", 2'000);
}

void debtDistress(const hh::stress::Config &config) {
  hh::stress::RunContext ctx{"economy-market", config};
  ctx.phase = "debt_distress";
  FinancingSystem financing;
  financing.setCurePeriodDays(2);
  LoanOffer offer;
  offer.id = 7;
  offer.principalCents = 1'200'000;
  offer.annualInterestBasisPoints = 1'200;
  offer.termMonths = 12;
  offer.paymentFrequencyDays = 30;
  offer.originationFeeCents = 12'000;
  offer.minimumCashCents = 100'000;
  if (!financing.acceptLoan(offer, 500'000).ok)
    ctx.fail("valid stress loan rejected", 0);
  const auto due = financing.processDay(30, 0);
  financing.observeDay(30, -1, 100'000, due.missedObligationCents > 0);
  financing.observeDay(32, -1, 100'000, true);
  const auto snapshot = financing.snapshot();
  if (snapshot.distressStage != DistressStage::Receivership ||
      snapshot.missedObligationCents <= 0)
    ctx.fail("uncured debt distress did not reach receivership", 32);
  const auto saved = financing.save();
  if (FinancingSystem::load(saved).save() != saved)
    ctx.fail("financing distress save/load diverged", 32);
}

void multiYearSoak(const hh::stress::Config &config,
                   const ScaleBudget &limits,
                   bool forceCrisis) {
  hh::stress::RunContext ctx{"economy-market", config};
  ctx.phase = forceCrisis ? "save_in_financial_crisis" : "multi_year_soak";
  const std::int64_t openingCash = forceCrisis ? 0 : 5'000'000'000LL;
  EconomyRuntime a(config.seed, openingCash);
  EconomyRuntime b(config.seed, openingCash);
  configureRuntime(a, limits);
  configureRuntime(b, limits);

  if (forceCrisis) {
    LoanOffer offer;
    offer.id = 100;
    offer.principalCents = 2'000'000;
    offer.annualInterestBasisPoints = 2'500;
    offer.termMonths = 6;
    offer.paymentFrequencyDays = 30;
    offer.originationFeeCents = 100'000;
    if (!a.acceptLoan(offer).ok || !b.acceptLoan(offer).ok)
      ctx.fail("financial-crisis loan fixture rejected", 0);
  }

  const int days = forceCrisis ? std::min(limits.soakDays, 180) : limits.soakDays;
  bool reloaded = false;
  for (int day = 0; day < days;) {
    const int chunk = std::min(30, days - day);
    a.runDays(chunk);
    b.runDays(chunk);
    day += chunk;
    assertRuntime(a, ctx, static_cast<std::uint64_t>(day));
    if (a.authoritativeHash() != b.authoritativeHash())
      ctx.fail("identical economy runtime replay diverged", day,
               std::to_string(a.authoritativeHash()) + "/" +
                   std::to_string(b.authoritativeHash()));
    if (!reloaded && day >= days / 2) {
      const auto saved = a.save();
      auto restored = EconomyRuntime::load(saved);
      if (restored.save() != saved ||
          restored.authoritativeHash() != a.authoritativeHash())
        ctx.fail("economy save/load did not preserve authoritative state", day);
      a = std::move(restored);
      reloaded = true;
      ctx.trace.push("economy:reload@" + std::to_string(day));
    }
  }

  const auto market = a.marketSnapshot();
  const auto finance = a.financialSnapshot();
  const auto inventory = a.revenueManagementSnapshot().inventory;
  const std::uint64_t processed = market.generatedRequests +
                                  finance.economics.transactionCount +
                                  static_cast<std::uint64_t>(inventory.bookings.size());
  if (!forceCrisis && config.scale == hh::stress::Scale::Pr &&
      processed < limits.operations)
    ctx.fail("Tier A economy campaign processed fewer than 50k operations", processed);
  if (a.currentDay() != days)
    ctx.fail("economy soak ended on wrong day", a.currentDay());
}

bool selected(const hh::stress::Config &config, std::string_view scenario) {
  return config.scenario.empty() || config.scenario == scenario;
}

void validateScenario(const hh::stress::Config &config) {
  if (config.scenario.empty())
    return;
  for (std::string_view valid : {
           "demand_spike", "future_book_saturation",
           "cancellation_no_show_storm", "competitor_rate_shock",
           "pricing_rule_overlap", "overbooking_recovery",
           "marketing_reputation_feedback", "corporate_group_pressure",
           "debt_distress", "multi_year_soak", "save_in_financial_crisis"})
    if (config.scenario == valid)
      return;
  throw std::invalid_argument(
      "unknown economy stress scenario; expected demand_spike, "
      "future_book_saturation, cancellation_no_show_storm, "
      "competitor_rate_shock, pricing_rule_overlap, overbooking_recovery, "
      "marketing_reputation_feedback, corporate_group_pressure, debt_distress, "
      "multi_year_soak, or save_in_financial_crisis");
}
} // namespace

int main() {
  try {
    const auto config =
        hh::stress::configFromEnvironment(0x5EED06065EED0606ULL);
    validateScenario(config);
    const auto limits = budget(config.scale);
    if (selected(config, "demand_spike"))
      demandSpike(config, limits.operations);
    if (selected(config, "future_book_saturation"))
      futureBookSaturation(config, limits.operations);
    if (selected(config, "cancellation_no_show_storm"))
      cancellationNoShowStorm(config);
    if (selected(config, "competitor_rate_shock"))
      competitorRateShock(config);
    if (selected(config, "pricing_rule_overlap"))
      pricingRuleOverlap(config, limits.operations);
    if (selected(config, "overbooking_recovery"))
      overbookingRecovery(config);
    if (selected(config, "marketing_reputation_feedback"))
      marketingReputationFeedback(config);
    if (selected(config, "corporate_group_pressure"))
      corporateGroupPressure(config);
    if (selected(config, "debt_distress"))
      debtDistress(config);
    if (selected(config, "multi_year_soak"))
      multiYearSoak(config, limits, false);
    if (selected(config, "save_in_financial_crisis"))
      multiYearSoak(config, limits, true);
    std::cout << "StressEconomyMarket PASS days=" << limits.soakDays
              << " operations=" << limits.operations << '\n';
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
