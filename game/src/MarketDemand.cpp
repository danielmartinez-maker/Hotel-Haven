#include "hh/game/MarketDemand.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace hh::game {
namespace {

constexpr double segmentPriceElasticity(MarketSegment segment) {
  switch (segment) {
  case MarketSegment::BudgetLeisure: return 1.60;
  case MarketSegment::Business: return 1.00;
  case MarketSegment::ExecutiveBusiness: return 0.72;
  case MarketSegment::CoupleLeisure: return 1.18;
  case MarketSegment::FamilyLeisure: return 1.30;
  case MarketSegment::LuxuryLeisure: return 0.58;
  case MarketSegment::ConferenceGroup: return 0.88;
  case MarketSegment::AirportTransit: return 1.42;
  case MarketSegment::Wellness: return 0.82;
  }
  return 1.0;
}

constexpr double segmentAmenitySensitivity(MarketSegment segment) {
  switch (segment) {
  case MarketSegment::BudgetLeisure: return 0.35;
  case MarketSegment::Business: return 1.00;
  case MarketSegment::ExecutiveBusiness: return 1.20;
  case MarketSegment::CoupleLeisure: return 0.90;
  case MarketSegment::FamilyLeisure: return 1.05;
  case MarketSegment::LuxuryLeisure: return 1.55;
  case MarketSegment::ConferenceGroup: return 1.10;
  case MarketSegment::AirportTransit: return 0.45;
  case MarketSegment::Wellness: return 1.70;
  }
  return 1.0;
}

std::int64_t baseBudget(MarketSegment segment) {
  switch (segment) {
  case MarketSegment::BudgetLeisure: return 11'000;
  case MarketSegment::Business: return 22'000;
  case MarketSegment::ExecutiveBusiness: return 32'000;
  case MarketSegment::CoupleLeisure: return 18'000;
  case MarketSegment::FamilyLeisure: return 20'000;
  case MarketSegment::LuxuryLeisure: return 36'000;
  case MarketSegment::ConferenceGroup: return 19'000;
  case MarketSegment::AirportTransit: return 13'000;
  case MarketSegment::Wellness: return 27'000;
  }
  return 17'000;
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
  return hotel.nightlyRateCents <= request.budgetCents + request.budgetCents / 2;
}

double MarketDemandSystem::priceUtility(const BookingRequest &request,
                                        const MarketHotelOffer &hotel) const {
  if (!isEligible(request, hotel))
    return 0.0;
  const double budget = static_cast<double>(request.budgetCents);
  const double ratio = static_cast<double>(hotel.nightlyRateCents) / budget;
  if (ratio <= 0.75)
    return 1.0;
  if (ratio <= 1.0)
    return 1.0 - ((ratio - 0.75) / 0.25) * 0.20;
  if (ratio <= 1.25)
    return 0.80 - ((ratio - 1.0) / 0.25) * 0.55;
  if (ratio <= 1.50)
    return std::max(0.0, 0.25 - ((ratio - 1.25) / 0.25) * 0.25);
  return 0.0;
}

double MarketDemandSystem::playerChoiceWeight(const BookingRequest &request,
                                               const MarketHotelOffer &hotel) const {
  if (!isEligible(request, hotel))
    return 0.0;
  const double basePriceUtility = priceUtility(request, hotel);
  if (basePriceUtility <= 0.0)
    return 0.0;
  const double price = std::pow(basePriceUtility, segmentPriceElasticity(request.segment));
  const double reputation = std::clamp(hotel.reputation, 0, 100) / 100.0;
  const double amenities = (std::clamp(hotel.amenityScore, 0, 100) / 100.0) *
                           segmentAmenitySensitivity(request.segment);
  const double location = std::clamp(hotel.locationScore, 0, 100) / 100.0;
  const double stars = std::clamp(hotel.stars, 1, 5) / 5.0;
  const double brand = std::clamp(hotel.brandScore, 0, 100) / 100.0;
  const double roomFit = request.partySize <= 4 ? 1.0 : 0.85;

  const double score = 0.30 * price + 0.20 * reputation + 0.15 * amenities +
                       0.10 * location + 0.10 * stars + 0.10 * brand +
                       0.05 * roomFit;
  constexpr double temperature = 0.35;
  return std::exp(std::clamp(score / temperature, -20.0, 20.0));
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
      MarketSegment::BudgetLeisure, MarketSegment::Business,
      MarketSegment::ExecutiveBusiness, MarketSegment::CoupleLeisure,
      MarketSegment::FamilyLeisure, MarketSegment::LuxuryLeisure,
      MarketSegment::ConferenceGroup, MarketSegment::AirportTransit,
      MarketSegment::Wellness};
  const int daySpan = lastArrivalDay - firstArrivalDay + 1;
  for (int i = 0; i < count; ++i) {
    BookingRequest request;
    request.id = nextRequestId_++;
    request.segment = segments[nextRandom() % 9];
    request.arrivalDay = firstArrivalDay +
        static_cast<int>(nextRandom() % static_cast<std::uint64_t>(daySpan));
    request.departureDay = request.arrivalDay + 1 + static_cast<int>(nextRandom() % 4);
    request.budgetCents = baseBudget(request.segment) +
                          static_cast<std::int64_t>(nextRandom() % 8001) - 2000;
    if (request.segment == MarketSegment::ConferenceGroup)
      request.partySize = 4 + static_cast<int>(nextRandom() % 8);
    else if (request.segment == MarketSegment::FamilyLeisure)
      request.partySize = 3 + static_cast<int>(nextRandom() % 4);
    else
      request.partySize = 1 + static_cast<int>(nextRandom() % 3);
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
