#include "hh/game/CommercialDemand.h"

#include <algorithm>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace hh::game {
namespace {

int reviewAlphaBasisPoints(std::uint64_t priorReviewCount) {
  constexpr int halfLife90AlphaBasisPoints = 77;
  const auto denominator = std::min<std::uint64_t>(priorReviewCount + 1, 1000000);
  const int lowVolumeAlpha = static_cast<int>(2000 / denominator);
  return std::max(halfLife90AlphaBasisPoints, lowVolumeAlpha);
}

int ewma(int previous, int sample, int alphaBasisPoints) {
  return std::clamp(
      (previous * (10000 - alphaBasisPoints) + sample * alphaBasisPoints + 5000) /
          10000,
      0, 10000);
}

bool validBasisPoints(int value) { return value >= 0 && value <= 10000; }
bool validOptionalBasisPoints(int value) {
  return value == -1 || validBasisPoints(value);
}
int reviewCategoryOrOverall(int category, int overall) {
  return category < 0 ? overall : category;
}

bool targets(const MarketingCampaign &campaign, MarketSegment segment) {
  return std::find(campaign.targetSegments.begin(), campaign.targetSegments.end(), segment) !=
         campaign.targetSegments.end();
}

int effectiveCampaignBoost(const MarketingCampaign &campaign, int day) {
  if (day < campaign.startDay)
    return 0;
  if (day <= campaign.endDay) {
    if (campaign.rampUpDays <= 0)
      return campaign.visibilityBoostBasisPoints;
    const int elapsedActiveDays = day - campaign.startDay + 1;
    if (elapsedActiveDays >= campaign.rampUpDays)
      return campaign.visibilityBoostBasisPoints;
    return campaign.visibilityBoostBasisPoints * elapsedActiveDays /
           campaign.rampUpDays;
  }
  if (campaign.attributionDecayDays <= 0)
    return 0;
  const int elapsedDecayDays = day - campaign.endDay;
  if (elapsedDecayDays >= campaign.attributionDecayDays)
    return 0;
  return campaign.visibilityBoostBasisPoints *
         (campaign.attributionDecayDays - elapsedDecayDays) /
         campaign.attributionDecayDays;
}

bool hasAmbiguousBilling(const MarketingCampaign &campaign) {
  return campaign.costCents > 0 && campaign.dailyCostCents > 0;
}

} // namespace

void CommercialDemand::setReputation(int basisPoints) {
  if (!validBasisPoints(basisPoints))
    throw std::invalid_argument("invalid reputation");
  snapshot_.overallReputationBasisPoints = basisPoints;
  snapshot_.serviceReputationBasisPoints = basisPoints;
  snapshot_.cleanlinessReputationBasisPoints = basisPoints;
  snapshot_.valueReputationBasisPoints = basisPoints;
  snapshot_.roomReputationBasisPoints = basisPoints;
  snapshot_.quietReputationBasisPoints = basisPoints;
  snapshot_.businessReputationBasisPoints = basisPoints;
  snapshot_.foodReputationBasisPoints = basisPoints;
  snapshot_.reviewCount = 0;
}

void CommercialDemand::applyReview(const ReviewSignal &review) {
  if (review.sourceId == 0 || !validBasisPoints(review.overallBasisPoints) ||
      !validBasisPoints(review.serviceBasisPoints) ||
      !validBasisPoints(review.cleanlinessBasisPoints) ||
      !validBasisPoints(review.valueBasisPoints) ||
      !validOptionalBasisPoints(review.roomBasisPoints) ||
      !validOptionalBasisPoints(review.quietBasisPoints) ||
      !validOptionalBasisPoints(review.businessBasisPoints) ||
      !validOptionalBasisPoints(review.foodBasisPoints))
    throw std::invalid_argument("invalid review signal");
  const int alphaBasisPoints = reviewAlphaBasisPoints(snapshot_.reviewCount);
  snapshot_.overallReputationBasisPoints =
      ewma(snapshot_.overallReputationBasisPoints, review.overallBasisPoints,
           alphaBasisPoints);
  snapshot_.serviceReputationBasisPoints =
      ewma(snapshot_.serviceReputationBasisPoints, review.serviceBasisPoints,
           alphaBasisPoints);
  snapshot_.cleanlinessReputationBasisPoints =
      ewma(snapshot_.cleanlinessReputationBasisPoints,
           review.cleanlinessBasisPoints, alphaBasisPoints);
  snapshot_.valueReputationBasisPoints =
      ewma(snapshot_.valueReputationBasisPoints, review.valueBasisPoints,
           alphaBasisPoints);
  snapshot_.roomReputationBasisPoints = ewma(
      snapshot_.roomReputationBasisPoints,
      reviewCategoryOrOverall(review.roomBasisPoints, review.overallBasisPoints),
      alphaBasisPoints);
  snapshot_.quietReputationBasisPoints = ewma(
      snapshot_.quietReputationBasisPoints,
      reviewCategoryOrOverall(review.quietBasisPoints, review.overallBasisPoints),
      alphaBasisPoints);
  snapshot_.businessReputationBasisPoints = ewma(
      snapshot_.businessReputationBasisPoints,
      reviewCategoryOrOverall(review.businessBasisPoints, review.overallBasisPoints),
      alphaBasisPoints);
  snapshot_.foodReputationBasisPoints = ewma(
      snapshot_.foodReputationBasisPoints,
      reviewCategoryOrOverall(review.foodBasisPoints, review.overallBasisPoints),
      alphaBasisPoints);
  ++snapshot_.reviewCount;
}

CommercialCommandResult CommercialDemand::startCampaign(const MarketingCampaign &campaign,
                                                         int currentDay) {
  if (campaign.id == 0 || campaign.startDay < currentDay ||
      campaign.endDay < campaign.startDay || campaign.costCents < 0 ||
      campaign.dailyCostCents < 0 || hasAmbiguousBilling(campaign) ||
      campaign.visibilityBoostBasisPoints < 0 || campaign.targetSegments.empty() ||
      campaign.rampUpDays < 0 || campaign.attributionDecayDays < 0 ||
      campaign.rampUpDays > 3650 || campaign.attributionDecayDays > 3650)
    return {false, "INVALID_MARKETING_CAMPAIGN", 0};
  if (std::any_of(snapshot_.campaigns.begin(), snapshot_.campaigns.end(),
                  [&](const auto &existing) { return existing.id == campaign.id; }))
    return {false, "DUPLICATE_CAMPAIGN_ID", 0};
  snapshot_.campaigns.push_back(campaign);
  std::sort(snapshot_.campaigns.begin(), snapshot_.campaigns.end(),
            [](const auto &a, const auto &b) { return a.id < b.id; });
  return {true, "OK", campaign.id};
}

int CommercialDemand::visibilityBasisPoints(MarketSegment segment, int day) const {
  int visibility = 10000;
  for (const auto &campaign : snapshot_.campaigns)
    if (targets(campaign, segment))
      visibility += effectiveCampaignBoost(campaign, day);
  return std::max(0, visibility);
}

ContractAcceptanceResult CommercialDemand::acceptContract(
    const CommercialContract &contract, const ContractFeasibility &feasibility,
    bool acceptRisk) {
  if (contract.id == 0 || contract.startDay < 0 ||
      contract.endDay < contract.startDay ||
      contract.endDay == std::numeric_limits<int>::max() ||
      contract.minimumRoomNights < 0 ||
      contract.maximumRoomNights < contract.minimumRoomNights ||
      contract.negotiatedRateCents <= 0 || contract.requiredVenueCapacity < 0 ||
      contract.requiredServiceUnits < 0 || contract.cancellationPenaltyCents < 0 ||
      contract.paymentDelayDays < 0)
    return {false, "INVALID_COMMERCIAL_CONTRACT", 0, false};
  if (std::any_of(snapshot_.contracts.begin(), snapshot_.contracts.end(),
                  [&](const auto &existing) {
                    return existing.contract.id == contract.id;
                  }))
    return {false, "DUPLICATE_CONTRACT_ID", 0, false};

  const bool feasible =
      feasibility.availableRoomNights >= contract.minimumRoomNights &&
      feasibility.venueCapacity >= contract.requiredVenueCapacity &&
      feasibility.serviceUnits >= contract.requiredServiceUnits;
  if (!feasible && !acceptRisk)
    return {false, "CURRENT_CAPACITY_INSUFFICIENT_ACCEPT_RISK_REQUIRED", 0, false};

  snapshot_.contracts.push_back({contract, !feasible, 0, 0});
  std::sort(snapshot_.contracts.begin(), snapshot_.contracts.end(),
            [](const auto &a, const auto &b) {
              return a.contract.id < b.contract.id;
            });
  return {true, feasible ? "OK" : "ACCEPTED_WITH_EXPLICIT_RISK", contract.id,
          !feasible};
}

void CommercialDemand::setContractCommitment(std::uint64_t contractId,
                                             int reservedRoomNights,
                                             int unfulfilledRoomNights) {
  if (contractId == 0 || reservedRoomNights < 0 || unfulfilledRoomNights < 0)
    throw std::invalid_argument("invalid commercial contract commitment");
  auto it = std::find_if(snapshot_.contracts.begin(), snapshot_.contracts.end(),
                         [&](const auto &accepted) {
                           return accepted.contract.id == contractId;
                         });
  if (it == snapshot_.contracts.end())
    throw std::invalid_argument("commercial contract not found");
  if (reservedRoomNights + unfulfilledRoomNights !=
      it->contract.minimumRoomNights)
    throw std::invalid_argument("commercial contract commitment does not reconcile");
  it->reservedRoomNights = reservedRoomNights;
  it->unfulfilledRoomNights = unfulfilledRoomNights;
}

CommercialDemandSnapshot CommercialDemand::snapshot() const { return snapshot_; }

std::string CommercialDemand::save() const {
  std::ostringstream out;
  out << "HHCOMM 6 " << snapshot_.overallReputationBasisPoints << ' '
      << snapshot_.serviceReputationBasisPoints << ' '
      << snapshot_.cleanlinessReputationBasisPoints << ' '
      << snapshot_.valueReputationBasisPoints << ' '
      << snapshot_.roomReputationBasisPoints << ' '
      << snapshot_.quietReputationBasisPoints << ' '
      << snapshot_.businessReputationBasisPoints << ' '
      << snapshot_.foodReputationBasisPoints << ' ' << snapshot_.reviewCount << ' '
      << snapshot_.campaigns.size();
  for (const auto &campaign : snapshot_.campaigns) {
    out << ' ' << campaign.id << ' ' << campaign.startDay << ' ' << campaign.endDay << ' '
        << campaign.costCents << ' ' << campaign.visibilityBoostBasisPoints << ' '
        << campaign.targetSegments.size();
    for (const auto segment : campaign.targetSegments)
      out << ' ' << static_cast<int>(segment);
    out << ' ' << campaign.rampUpDays << ' ' << campaign.attributionDecayDays << ' '
        << campaign.dailyCostCents;
  }
  out << ' ' << snapshot_.contracts.size();
  for (const auto &accepted : snapshot_.contracts) {
    const auto &c = accepted.contract;
    out << ' ' << c.id << ' ' << c.startDay << ' ' << c.endDay << ' '
        << c.minimumRoomNights << ' ' << c.maximumRoomNights << ' '
        << c.negotiatedRateCents << ' ' << c.requiredVenueCapacity << ' '
        << c.requiredServiceUnits << ' ' << c.cancellationPenaltyCents << ' '
        << c.paymentDelayDays << ' ' << accepted.acceptedRisk << ' '
        << accepted.reservedRoomNights << ' ' << accepted.unfulfilledRoomNights;
  }
  return out.str();
}

CommercialDemand CommercialDemand::load(std::string_view data) {
  std::istringstream in{std::string(data)};
  std::string magic;
  int version{};
  std::size_t count{};
  CommercialDemand result;
  in >> magic >> version >> result.snapshot_.overallReputationBasisPoints >>
      result.snapshot_.serviceReputationBasisPoints >>
      result.snapshot_.cleanlinessReputationBasisPoints >>
      result.snapshot_.valueReputationBasisPoints;
  if (!in || magic != "HHCOMM" ||
      (version != 1 && version != 2 && version != 3 && version != 4 &&
       version != 5 && version != 6))
    throw std::invalid_argument("invalid commercial demand save");
  if (version >= 5) {
    in >> result.snapshot_.roomReputationBasisPoints >>
        result.snapshot_.quietReputationBasisPoints >>
        result.snapshot_.businessReputationBasisPoints >>
        result.snapshot_.foodReputationBasisPoints;
  } else {
    result.snapshot_.roomReputationBasisPoints =
        result.snapshot_.overallReputationBasisPoints;
    result.snapshot_.quietReputationBasisPoints =
        result.snapshot_.overallReputationBasisPoints;
    result.snapshot_.businessReputationBasisPoints =
        result.snapshot_.overallReputationBasisPoints;
    result.snapshot_.foodReputationBasisPoints =
        result.snapshot_.overallReputationBasisPoints;
  }
  if (version >= 2)
    in >> result.snapshot_.reviewCount;
  if (!in || !validBasisPoints(result.snapshot_.overallReputationBasisPoints) ||
      !validBasisPoints(result.snapshot_.serviceReputationBasisPoints) ||
      !validBasisPoints(result.snapshot_.cleanlinessReputationBasisPoints) ||
      !validBasisPoints(result.snapshot_.valueReputationBasisPoints) ||
      !validBasisPoints(result.snapshot_.roomReputationBasisPoints) ||
      !validBasisPoints(result.snapshot_.quietReputationBasisPoints) ||
      !validBasisPoints(result.snapshot_.businessReputationBasisPoints) ||
      !validBasisPoints(result.snapshot_.foodReputationBasisPoints))
    throw std::invalid_argument("invalid saved commercial reputation");

  in >> count;
  if (!in || count > 100000)
    throw std::invalid_argument("invalid commercial demand save");
  for (std::size_t i = 0; i < count; ++i) {
    MarketingCampaign campaign;
    std::size_t segmentCount{};
    in >> campaign.id >> campaign.startDay >> campaign.endDay >> campaign.costCents >>
        campaign.visibilityBoostBasisPoints >> segmentCount;
    if (!in || segmentCount > 16)
      throw std::invalid_argument("invalid saved campaign");
    for (std::size_t j = 0; j < segmentCount; ++j) {
      int segment{};
      in >> segment;
      if (!in || segment < static_cast<int>(MarketSegment::CoupleLeisure) ||
          segment > static_cast<int>(MarketSegment::Wellness))
        throw std::invalid_argument("invalid saved market segment");
      campaign.targetSegments.push_back(static_cast<MarketSegment>(segment));
    }
    if (version >= 3)
      in >> campaign.rampUpDays >> campaign.attributionDecayDays;
    if (version >= 6)
      in >> campaign.dailyCostCents;
    if (!in || campaign.rampUpDays < 0 || campaign.attributionDecayDays < 0 ||
        campaign.dailyCostCents < 0 || hasAmbiguousBilling(campaign))
      throw std::invalid_argument("invalid saved campaign schedule");
    result.snapshot_.campaigns.push_back(std::move(campaign));
  }
  in >> count;
  if (!in || count > 100000)
    throw std::invalid_argument("invalid contract count");
  for (std::size_t i = 0; i < count; ++i) {
    AcceptedCommercialContract accepted;
    auto &c = accepted.contract;
    in >> c.id >> c.startDay >> c.endDay >> c.minimumRoomNights >>
        c.maximumRoomNights >> c.negotiatedRateCents >> c.requiredVenueCapacity >>
        c.requiredServiceUnits >> c.cancellationPenaltyCents >> c.paymentDelayDays >>
        accepted.acceptedRisk;
    if (version >= 4) {
      in >> accepted.reservedRoomNights >> accepted.unfulfilledRoomNights;
    } else {
      accepted.reservedRoomNights = 0;
      accepted.unfulfilledRoomNights = c.minimumRoomNights;
    }
    if (!in || accepted.reservedRoomNights < 0 || accepted.unfulfilledRoomNights < 0 ||
        accepted.reservedRoomNights + accepted.unfulfilledRoomNights !=
            c.minimumRoomNights)
      throw std::invalid_argument("invalid saved contract");
    result.snapshot_.contracts.push_back(std::move(accepted));
  }
  in >> std::ws;
  if (!in.eof())
    throw std::invalid_argument("unexpected commercial demand trailing data");
  return result;
}

} // namespace hh::game
