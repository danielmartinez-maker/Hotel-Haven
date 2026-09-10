#pragma once

#include "hh/game/MarketDemand.h"
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace hh::game {

struct ReviewSignal {
  std::uint64_t sourceId{};
  int overallBasisPoints{};
  int serviceBasisPoints{};
  int cleanlinessBasisPoints{};
  int valueBasisPoints{};
};

struct MarketingCampaign {
  std::uint64_t id{};
  int startDay{};
  int endDay{};
  std::int64_t costCents{};
  int visibilityBoostBasisPoints{};
  std::vector<MarketSegment> targetSegments;
};

struct CommercialCommandResult {
  bool ok{};
  std::string reason;
  std::uint64_t id{};
  explicit operator bool() const noexcept { return ok; }
};

struct CommercialContract {
  std::uint64_t id{};
  int startDay{};
  int endDay{};
  int minimumRoomNights{};
  int maximumRoomNights{};
  std::int64_t negotiatedRateCents{};
  int requiredVenueCapacity{};
  int requiredServiceUnits{};
  std::int64_t cancellationPenaltyCents{};
  int paymentDelayDays{};
};

struct ContractFeasibility {
  int availableRoomNights{};
  int venueCapacity{};
  int serviceUnits{};
};

struct ContractAcceptanceResult {
  bool ok{};
  std::string reason;
  std::uint64_t contractId{};
  bool acceptedRisk{};
  explicit operator bool() const noexcept { return ok; }
};

struct AcceptedCommercialContract {
  CommercialContract contract;
  bool acceptedRisk{};
};

struct CommercialDemandSnapshot {
  int overallReputationBasisPoints{7000};
  int serviceReputationBasisPoints{7000};
  int cleanlinessReputationBasisPoints{7000};
  int valueReputationBasisPoints{7000};
  std::uint64_t reviewCount{};
  std::vector<MarketingCampaign> campaigns;
  std::vector<AcceptedCommercialContract> contracts;
};

class CommercialDemand {
public:
  void setReputation(int basisPoints);
  void applyReview(const ReviewSignal &review);
  [[nodiscard]] CommercialCommandResult startCampaign(const MarketingCampaign &campaign,
                                                       int currentDay);
  [[nodiscard]] int visibilityBasisPoints(MarketSegment segment, int day) const;
  [[nodiscard]] ContractAcceptanceResult acceptContract(
      const CommercialContract &contract, const ContractFeasibility &feasibility,
      bool acceptRisk);
  [[nodiscard]] CommercialDemandSnapshot snapshot() const;
  [[nodiscard]] std::string save() const;
  static CommercialDemand load(std::string_view data);

private:
  CommercialDemandSnapshot snapshot_{};
};

} // namespace hh::game
