#pragma once

#include "hh/game/CommercialDemand.h"
#include "hh/game/Financing.h"
#include "hh/game/HotelEconomics.h"
#include "hh/game/MarketDemand.h"
#include "hh/game/Overbooking.h"
#include "hh/game/RevenueInventory.h"
#include "hh/game/RevenueManagement.h"
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace hh::game {

struct FinancialSnapshot {
  HotelEconomicsSnapshot economics;
  FinancingSnapshot financing;
};

struct EconomyDiagnostics {
  int occupancyTodayBasisPoints{};
  int occupancy7DayBasisPoints{};
  int occupancy30DayBasisPoints{};
  std::int64_t adrCents{};
  std::int64_t revParCents{};
  std::int64_t roomRevenueCents{};
  std::int64_t totalRevenueCents{};
  std::int64_t laborCostCents{};
  int laborCostShareBasisPoints{};
  std::int64_t utilityCostCents{};
  std::int64_t foodBeverageCostCents{};
  int foodCostShareBasisPoints{};
  std::int64_t channelCommissionCents{};
  std::int64_t competitorMedianRateCents{};
  std::uint64_t cancellations{};
  std::uint64_t noShows{};
  std::map<int, int> bookingPaceByArrivalDay;
  std::map<BookingChannel, std::uint64_t> channelBookings;
  std::map<MarketSegment, std::uint64_t> demandBySegment;
  double cashRunwayDays{};
  std::int64_t outstandingPrincipalCents{};
  std::int64_t nextDebtServiceCents{};
  int nextDebtPaymentDay{};
};

class EconomyRuntime {
public:
  explicit EconomyRuntime(std::uint64_t seed = 1,
                          std::int64_t openingCashCents = 5'000'000);

  void setPhysicalRoomCapacity(std::string category, int units);
  void setPlayerHotelOffer(const MarketHotelOffer &offer);
  void setCompetitors(std::vector<CompetitorOffer> competitors);

  [[nodiscard]] PricingRuleResult setPricingRule(const PricingRuleCommand &command);
  [[nodiscard]] OverbookingResult setOverbookingPolicy(const OverbookingPolicy &policy);
  [[nodiscard]] LoanResult acceptLoan(const LoanOffer &offer);
  [[nodiscard]] CommercialCommandResult startMarketingCampaign(
      const MarketingCampaign &campaign);
  [[nodiscard]] ContractAcceptanceResult acceptCommercialContract(
      const CommercialContract &contract, const ContractFeasibility &feasibility,
      bool acceptRisk = false);
  void applyReviewOutcome(const ReviewSignal &review);

  // Integration hooks for an owning campaign runtime. These do not generate
  // market requests or reservations; they only mirror authoritative external
  // economic events and absolute KPI counters into the FINAL-06 ledger.
  void postExternalTransaction(int day, EconomicCategory category,
                               std::int64_t amountCents,
                               std::uint64_t sourceId, std::string memo);
  void synchronizeExternalMetrics(int currentDay,
                                  std::int64_t cumulativeSellableRoomNights,
                                  std::int64_t cumulativeOccupiedRoomNights);

  void runDays(int days);

  [[nodiscard]] MarketSnapshot marketSnapshot() const;
  [[nodiscard]] RevenueManagementSnapshot revenueManagementSnapshot() const;
  [[nodiscard]] FinancialSnapshot financialSnapshot() const;
  [[nodiscard]] CommercialDemandSnapshot commercialSnapshot() const;
  [[nodiscard]] EconomyDiagnostics diagnostics() const;
  [[nodiscard]] int currentDay() const noexcept;

  [[nodiscard]] std::string save() const;
  static EconomyRuntime load(std::string_view data);
  [[nodiscard]] std::uint64_t authoritativeHash() const;
  [[nodiscard]] std::size_t estimatedStateBytes() const;

private:
  std::uint64_t seed_{1};
  int currentDay_{};
  std::uint64_t nextBookingId_{1};
  std::uint64_t nextTransactionId_{1};
  std::int64_t cumulativeSellableRoomNights_{};
  std::int64_t cumulativeOccupiedRoomNights_{};
  std::map<std::string, int> physicalCapacity_;
  MarketHotelOffer basePlayerOffer_{};

  MarketDemandSystem market_;
  RevenueInventory inventory_;
  RevenueManagement revenueManagement_;
  HotelEconomics economics_;
  FinancingSystem financing_;
  OverbookingSystem overbooking_;
  CommercialDemand commercial_;

  void runOneDay();
  [[nodiscard]] int physicalCapacityTotal() const;
  [[nodiscard]] std::int64_t currentCashCents() const;
  void post(EconomicCategory category, std::int64_t amountCents,
            std::uint64_t sourceId, std::string memo);
};

} // namespace hh::game
