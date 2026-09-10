#include "hh/game/CommercialDemand.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace hh::game {
namespace {

int reviewAlphaBasisPoints(std::uint64_t priorReviewCount) {
  // 77 bp is the asymptotic EWMA alpha corresponding to an effective
  // half-life of roughly 90 review-equivalents. Low-volume hotels react more
  // quickly so an individual review remains meaningful early in a campaign.
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

bool targets(const MarketingCampaign &campaign, MarketSegment segment) {
  return std::find(campaign.targetSegments.begin(), campaign.targetSegments.end(), segment) !=
         campaign.targetSegments.end();
}

} // namespace

void CommercialDemand::setReputation(int basisPoints) {
  if (basisPoints < 0 || basisPoints > 10000)
    throw std::invalid_argument("invalid reputation");
  snapshot_.overallReputationBasisPoints = basisPoints;
  snapshot_.serviceReputationBasisPoints = basisPoints;
  snapshot_.cleanlinessReputationBasisPoints = basisPoints;
  snapshot_.valueReputationBasisPoints = basisPoints;
  snapshot_.reviewCount = 0;
}

void CommercialDemand::applyReview(const ReviewSignal &review) {
  if (review.sourceId == 0 || review.overallBasisPoints < 0 ||
      review.overallBasisPoints > 10000 || review.serviceBasisPoints < 0 ||
      review.serviceBasisPoints > 10000 || review.cleanlinessBasisPoints < 0 ||
      review.cleanlinessBasisPoints > 10000 || review.valueBasisPoints < 0 ||
      review.valueBasisPoints > 10000)
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
  ++snapshot_.reviewCount;
}

CommercialCommandResult CommercialDemand::startCampaign(const MarketingCampaign &campaign,
                                                         int currentDay) {
  if (campaign.id == 0 || campaign.startDay < currentDay ||
      campaign.endDay < campaign.startDay || campaign.costCents < 0 ||
      campaign.visibilityBoostBasisPoints < 0 || campaign.targetSegments.empty())
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
    if (day >= campaign.startDay && day <= campaign.endDay && targets(campaign, segment))
      visibility += campaign.visibilityBoostBasisPoints;
  return std::max(0, visibility);
}

ContractAcceptanceResult CommercialDemand::acceptContract(
    const CommercialContract &contract, const ContractFeasibility &feasibility,
    bool acceptRisk) {
  if (contract.id == 0 || contract.startDay < 0 || contract.endDay < contract.startDay ||
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

  snapshot_.contracts.push_back({contract, !feasible});
  std::sort(snapshot_.contracts.begin(), snapshot_.contracts.end(),
            [](const auto &a, const auto &b) {
              return a.contract.id < b.contract.id;
            });
  return {true, feasible ? "OK" : "ACCEPTED_WITH_EXPLICIT_RISK", contract.id,
          !feasible};
}

CommercialDemandSnapshot CommercialDemand::snapshot() const { return snapshot_; }

std::string CommercialDemand::save() const {
  std::ostringstream out;
  out << "HHCOMM 2 " << snapshot_.overallReputationBasisPoints << ' '
      << snapshot_.serviceReputationBasisPoints << ' '
      << snapshot_.cleanlinessReputationBasisPoints << ' '
      << snapshot_.valueReputationBasisPoints << ' ' << snapshot_.reviewCount << ' '
      << snapshot_.campaigns.size();
  for (const auto &campaign : snapshot_.campaigns) {
    out << ' ' << campaign.id << ' ' << campaign.startDay << ' ' << campaign.endDay << ' '
        << campaign.costCents << ' ' << campaign.visibilityBoostBasisPoints << ' '
        << campaign.targetSegments.size();
    for (const auto segment : campaign.targetSegments)
      out << ' ' << static_cast<int>(segment);
  }
  out << ' ' << snapshot_.contracts.size();
  for (const auto &accepted : snapshot_.contracts) {
    const auto &c = accepted.contract;
    out << ' ' << c.id << ' ' << c.startDay << ' ' << c.endDay << ' '
        << c.minimumRoomNights << ' ' << c.maximumRoomNights << ' '
        << c.negotiatedRateCents << ' ' << c.requiredVenueCapacity << ' '
        << c.requiredServiceUnits << ' ' << c.cancellationPenaltyCents << ' '
        << c.paymentDelayDays << ' ' << accepted.acceptedRisk;
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
  if (!in || magic != "HHCOMM" || (version != 1 && version != 2))
    throw std::invalid_argument("invalid commercial demand save");
  if (version >= 2)
    in >> result.snapshot_.reviewCount;
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
    if (!in)
      throw std::invalid_argument("invalid saved contract");
    result.snapshot_.contracts.push_back(std::move(accepted));
  }
  in >> std::ws;
  if (!in.eof())
    throw std::invalid_argument("unexpected commercial demand trailing data");
  return result;
}

} // namespace hh::game
