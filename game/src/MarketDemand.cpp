#include "hh/game/MarketDemand.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace hh::game {
namespace {

constexpr double segmentPriceSensitivity(MarketSegment segment) {
  switch (segment) {
  case MarketSegment::Budget: return 3.0;
  case MarketSegment::Leisure: return 2.0;
  case MarketSegment::Group: return 1.6;
  case MarketSegment::Business: return 1.1;
  case MarketSegment::Luxury: return 0.7;
  }
  return 1.0;
}

constexpr double segmentAmenitySensitivity(MarketSegment segment) {
  switch (segment) {
  case MarketSegment::Luxury: return 1.5;
  case MarketSegment::Business: return 1.0;
  case MarketSegment::Group: return 0.9;
  case MarketSegment::Leisure: return 0.8;
  case MarketSegment::Budget: return 0.4;
  }
  return 1.0;
}

MarketHotelOffer asHotel(const CompetitorOffer &c) {
  return {c.hotelId, c.nightlyRateCents, c.reputation, c.stars,
          c.amenityScore, c.locationScore, c.brandScore, true};
}

} // namespace

MarketDemandSystem::MarketDemandSystem(std::uint64_t seed)
    : seed_(seed ? seed : 1), rngState_(seed_ ^ 0x9E3779B97F4A7C15ULL) {}

void MarketDemandSystem::setPlayerOffer(const MarketHotelOffer &offer) {
  player_ = offer;
  refreshComparableMedian();
}

void MarketDemandSystem::setCompetitors(std::vector<CompetitorOffer> competitors) {
  std::sort(competitors.begin(), competitors.end(), [](const auto &a, const auto &b) {
    return a.hotelId < b.hotelId;
  });
  competitors_ = std::move(competitors);
  snapshot_.competitors = competitors_;
  snapshot_.physicalCompetitorGuests = 0;
  refreshComparableMedian();
}

std::uint64_t MarketDemandSystem::nextRandom() {
  rngState_ += 0x9E3779B97F4A7C15ULL;
  std::uint64_t z = rngState_;
  z = (z ^ (z >> 30U)) * 0xBF58476D1CE4E5B9ULL;
  z = (z ^ (z >> 27U)) * 0x94D049BB133111EBULL;
  return z ^ (z >> 31U);
}

double MarketDemandSystem::unitRandom() {
  return static_cast<double>(nextRandom() >> 11U) * (1.0 / 9007199254740992.0);
}

bool MarketDemandSystem::isEligible(const BookingRequest &request,
                                    const MarketHotelOffer &hotel) const {
  if (!hotel.sellable || hotel.hotelId == 0 || request.budgetCents <= 0 ||
      hotel.nightlyRateCents <= 0)
    return false;
  // HMG-030 default hard consideration cutoff: strictly above 1.5x budget.
  return hotel.nightlyRateCents <= request.budgetCents + request.budgetCents / 2;
}

double MarketDemandSystem::playerChoiceWeight(const BookingRequest &request,
                                               const MarketHotelOffer &hotel) const {
  if (!isEligible(request, hotel))
    return 0.0;
  const double budget = static_cast<double>(std::max<std::int64_t>(1, request.budgetCents));
  const double rateRatio = static_cast<double>(hotel.nightlyRateCents) / budget;
  const double priceUtility = -segmentPriceSensitivity(request.segment) * rateRatio;
  const double reputationUtility = std::clamp(hotel.reputation, 0, 100) * 0.018;
  const double starUtility = std::clamp(hotel.stars, 1, 5) * 0.12;
  const double amenityFit = (std::clamp(hotel.amenityScore, 0, 100) / 100.0) *
                            segmentAmenitySensitivity(request.segment);
  const double locationFit = std::clamp(hotel.locationScore, 0, 100) / 100.0 * 0.65;
  const double brandFit = std::clamp(hotel.brandScore, 0, 100) / 100.0 * 0.35;
  const double utility = priceUtility + reputationUtility + starUtility + amenityFit +
                         locationFit + brandFit;
  return std::exp(std::clamp(utility, -20.0, 20.0));
}

void MarketDemandSystem::allocate(const BookingRequest &request) {
  struct Candidate { std::uint64_t id{}; double weight{}; bool player{}; };
  std::vector<Candidate> candidates;
  const double playerWeight = playerChoiceWeight(request, player_);
  if (playerWeight > 0.0)
    candidates.push_back({player_.hotelId, playerWeight, true});
  for (const auto &competitor : competitors_) {
    const auto offer = asHotel(competitor);
    const double weight = playerChoiceWeight(request, offer);
    if (weight > 0.0)
      candidates.push_back({offer.hotelId, weight, false});
  }
  std::sort(candidates.begin(), candidates.end(), [](const auto &a, const auto &b) {
    return a.id < b.id;
  });
  if (candidates.empty()) {
    ++snapshot_.unallocatedRequests;
    snapshot_.choices.push_back({request.id, 0, false});
    return;
  }
  double total = 0.0;
  for (const auto &candidate : candidates)
    total += candidate.weight;
  double cursor = unitRandom() * total;
  const Candidate *chosen = &candidates.back();
  for (const auto &candidate : candidates) {
    if (cursor < candidate.weight) {
      chosen = &candidate;
      break;
    }
    cursor -= candidate.weight;
  }
  snapshot_.choices.push_back({request.id, chosen->id, chosen->player});
  if (chosen->player)
    ++snapshot_.playerWins;
  else
    ++snapshot_.competitorWins;
}

void MarketDemandSystem::generateRequests(int firstArrivalDay, int lastArrivalDay, int count) {
  if (count <= 0 || lastArrivalDay < firstArrivalDay)
    return;
  static constexpr MarketSegment segments[] = {
      MarketSegment::Leisure, MarketSegment::Business, MarketSegment::Group,
      MarketSegment::Luxury, MarketSegment::Budget};
  const int daySpan = lastArrivalDay - firstArrivalDay + 1;
  for (int i = 0; i < count; ++i) {
    BookingRequest request;
    request.id = nextRequestId_++;
    request.segment = segments[nextRandom() % 5];
    request.arrivalDay = firstArrivalDay + static_cast<int>(nextRandom() % static_cast<std::uint64_t>(daySpan));
    request.departureDay = request.arrivalDay + 1 + static_cast<int>(nextRandom() % 4);
    const std::int64_t baseBudget = request.segment == MarketSegment::Luxury ? 30000
                                    : request.segment == MarketSegment::Business ? 22000
                                    : request.segment == MarketSegment::Group ? 18000
                                    : request.segment == MarketSegment::Budget ? 11000
                                                                              : 17000;
    request.budgetCents = baseBudget + static_cast<std::int64_t>(nextRandom() % 8001) - 2000;
    request.partySize = request.segment == MarketSegment::Group
                            ? 4 + static_cast<int>(nextRandom() % 5)
                            : 1 + static_cast<int>(nextRandom() % 3);
    request.amenityPreference = static_cast<int>(nextRandom() % 101);
    request.locationPreference = static_cast<int>(nextRandom() % 101);
    request.brandPreference = static_cast<int>(nextRandom() % 101);
    snapshot_.requests.push_back(request);
    ++snapshot_.generatedRequests;
    allocate(request);
  }
}

void MarketDemandSystem::refreshComparableMedian() {
  std::vector<std::int64_t> rates;
  rates.reserve(competitors_.size());
  for (const auto &competitor : competitors_)
    if (competitor.nightlyRateCents > 0)
      rates.push_back(competitor.nightlyRateCents);
  if (rates.empty()) {
    snapshot_.comparableMedianRateCents = player_.nightlyRateCents;
    return;
  }
  std::sort(rates.begin(), rates.end());
  const auto middle = rates.size() / 2;
  snapshot_.comparableMedianRateCents = rates.size() % 2
      ? rates[middle]
      : (rates[middle - 1] + rates[middle]) / 2;
}

MarketSnapshot MarketDemandSystem::snapshot() const {
  auto result = snapshot_;
  result.competitors = competitors_;
  result.physicalCompetitorGuests = 0;
  return result;
}

} // namespace hh::game
