#include "hh/game/MarketDemand.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <stdexcept>

namespace hh::game {
namespace {

constexpr MarketSegment kSegments[] = {
    MarketSegment::BudgetLeisure, MarketSegment::Business,
    MarketSegment::ExecutiveBusiness, MarketSegment::CoupleLeisure,
    MarketSegment::FamilyLeisure, MarketSegment::LuxuryLeisure,
    MarketSegment::ConferenceGroup, MarketSegment::AirportTransit,
    MarketSegment::Wellness};
constexpr int kDefaultPlayerConsiderationBasisPoints = 7000;

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

SegmentDemandProfile baselineProfile(MarketSegment segment) {
  SegmentDemandProfile profile;
  profile.segment = segment;
  profile.baseBudgetCents = baseBudget(segment);
  profile.priceElasticityBasisPoints =
      static_cast<int>(std::llround(segmentPriceElasticity(segment) * 10000.0));
  profile.amenitySensitivityBasisPoints =
      static_cast<int>(std::llround(segmentAmenitySensitivity(segment) * 10000.0));
  switch (segment) {
  case MarketSegment::BudgetLeisure:
    profile.baseDailyDemand = 18.0;
    profile.medianLeadTimeDays = 14;
    profile.medianStayNights = 2;
    break;
  case MarketSegment::Business:
    profile.baseDailyDemand = 22.0;
    profile.medianLeadTimeDays = 10;
    profile.medianStayNights = 2;
    break;
  case MarketSegment::ExecutiveBusiness:
    profile.baseDailyDemand = 8.0;
    profile.medianLeadTimeDays = 14;
    profile.medianStayNights = 2;
    break;
  case MarketSegment::CoupleLeisure:
    profile.baseDailyDemand = 20.0;
    profile.medianLeadTimeDays = 21;
    profile.medianStayNights = 3;
    break;
  case MarketSegment::FamilyLeisure:
    profile.baseDailyDemand = 14.0;
    profile.medianLeadTimeDays = 35;
    profile.medianStayNights = 4;
    break;
  case MarketSegment::LuxuryLeisure:
    profile.baseDailyDemand = 7.0;
    profile.medianLeadTimeDays = 28;
    profile.medianStayNights = 3;
    break;
  case MarketSegment::ConferenceGroup:
    profile.baseDailyDemand = 9.0;
    profile.medianLeadTimeDays = 120;
    profile.medianStayNights = 3;
    break;
  case MarketSegment::AirportTransit:
    profile.baseDailyDemand = 15.0;
    profile.medianLeadTimeDays = 2;
    profile.medianStayNights = 1;
    break;
  case MarketSegment::Wellness:
    profile.baseDailyDemand = 6.0;
    profile.medianLeadTimeDays = 24;
    profile.medianStayNights = 3;
    break;
  }
  return profile;
}

int defaultPartySize(MarketSegment segment) {
  if (segment == MarketSegment::ConferenceGroup)
    return 6;
  if (segment == MarketSegment::FamilyLeisure)
    return 4;
  return 2;
}

bool validBasisPoints(int value) {
  return value >= 0 && value <= 100000;
}

bool validCategoryScore(int value) {
  return value == -1 || (value >= 0 && value <= 100);
}

bool validReputationCategories(const ReputationCategoryScores &scores) {
  return validCategoryScore(scores.service) && validCategoryScore(scores.room) &&
         validCategoryScore(scores.cleanliness) && validCategoryScore(scores.quiet) &&
         validCategoryScore(scores.business) && validCategoryScore(scores.food);
}

bool validChoiceWeights(const SegmentChoiceWeights &weights) {
  return weights.priceBasisPoints >= 0 && weights.reputationBasisPoints >= 0 &&
         weights.amenityBasisPoints >= 0 && weights.locationBasisPoints >= 0 &&
         weights.starBasisPoints >= 0 && weights.roomBasisPoints >= 0 &&
         weights.brandBasisPoints >= 0 && weights.totalBasisPoints() == 10000;
}

int categoryOrOverall(int category, int overall) {
  return category < 0 ? std::clamp(overall, 0, 100) : category;
}

bool validCompetitorCalendar(const CompetitorOffer &competitor) {
  if (competitor.roomCount < 0)
    return false;
  std::set<std::string> declaredCategories;
  for (const auto &category : competitor.roomCategories) {
    if (category.empty() || !declaredCategories.insert(category).second)
      return false;
  }
  std::vector<CompetitorInventoryWindow> windows = competitor.inventoryWindows;
  std::sort(windows.begin(), windows.end(), [](const auto &a, const auto &b) {
    if (a.roomCategory != b.roomCategory)
      return a.roomCategory < b.roomCategory;
    if (a.startDay != b.startDay)
      return a.startDay < b.startDay;
    return a.endDay < b.endDay;
  });
  for (std::size_t i = 0; i < windows.size(); ++i) {
    const auto &window = windows[i];
    if (window.roomCategory.empty() || window.startDay < 0 ||
        window.endDay < window.startDay || window.nightlyRateCents <= 0 ||
        window.availableRooms < 0 ||
        (!declaredCategories.empty() &&
         !declaredCategories.contains(window.roomCategory)))
      return false;
    if (i > 0 && windows[i - 1].roomCategory == window.roomCategory &&
        window.startDay <= windows[i - 1].endDay)
      return false;
  }
  return true;
}

std::uint64_t mix64(std::uint64_t value) {
  value = (value ^ (value >> 30U)) * 0xBF58476D1CE4E5B9ULL;
  value = (value ^ (value >> 27U)) * 0x94D049BB133111EBULL;
  return value ^ (value >> 31U);
}

} // namespace

MarketDemandSystem::MarketDemandSystem(std::uint64_t seed)
    : seed_(seed ? seed : 1), rngState_(seed_ ^ 0x9E3779B97F4A7C15ULL) {
  for (const auto segment : kSegments) {
    demandProfiles_[segment] = baselineProfile(segment);
    playerConsiderationBasisPoints_[segment] = kDefaultPlayerConsiderationBasisPoints;
  }
}

void MarketDemandSystem::setPlayerOffer(const MarketHotelOffer &offer) {
  if (!validReputationCategories(offer.reputationCategories))
    throw std::invalid_argument("invalid player reputation categories");
  player_ = offer;
  refreshComparableMedian();
}

void MarketDemandSystem::setCompetitors(std::vector<CompetitorOffer> competitors) {
  if (std::any_of(competitors.begin(), competitors.end(), [](const auto &competitor) {
        return competitor.hotelId == 0 || competitor.nightlyRateCents <= 0 ||
               !validReputationCategories(competitor.reputationCategories) ||
               !validCompetitorCalendar(competitor);
      }))
    throw std::invalid_argument("invalid competitor offer");
  for (auto &competitor : competitors) {
    std::sort(competitor.roomCategories.begin(), competitor.roomCategories.end());
    std::sort(competitor.inventoryWindows.begin(), competitor.inventoryWindows.end(),
              [](const auto &a, const auto &b) {
                if (a.roomCategory != b.roomCategory)
                  return a.roomCategory < b.roomCategory;
                if (a.startDay != b.startDay)
                  return a.startDay < b.startDay;
                return a.endDay < b.endDay;
              });
  }
  std::sort(competitors.begin(), competitors.end(), [](const auto &a, const auto &b) {
    return a.hotelId < b.hotelId;
  });
  for (std::size_t i = 1; i < competitors.size(); ++i)
    if (competitors[i - 1].hotelId == competitors[i].hotelId)
      throw std::invalid_argument("duplicate competitor hotel id");
  competitors_ = std::move(competitors);
  snapshot_.competitors = competitors_;
  snapshot_.physicalCompetitorGuests = 0;
  refreshComparableMedian();
}

void MarketDemandSystem::setSegmentDemandProfile(const SegmentDemandProfile &profile) {
  if (!std::isfinite(profile.baseDailyDemand) || profile.baseDailyDemand < 0.0 ||
      profile.medianLeadTimeDays < 0 || profile.medianStayNights <= 0 ||
      profile.baseBudgetCents <= 0 || profile.priceElasticityBasisPoints < 0 ||
      profile.priceElasticityBasisPoints > 100000 ||
      profile.amenitySensitivityBasisPoints < 0 ||
      profile.amenitySensitivityBasisPoints > 100000 ||
      profile.cancellationBasisPoints < 0 || profile.cancellationBasisPoints > 10000 ||
      profile.noShowBasisPoints < 0 || profile.noShowBasisPoints > 10000 ||
      !validChoiceWeights(profile.choiceWeights) ||
      std::any_of(profile.weekdayMultiplierBasisPoints.begin(),
                  profile.weekdayMultiplierBasisPoints.end(),
                  [](int value) { return !validBasisPoints(value); }))
    throw std::invalid_argument("invalid segment demand profile");
  demandProfiles_[profile.segment] = profile;
}

const SegmentDemandProfile *MarketDemandSystem::profileFor(MarketSegment segment) const {
  const auto it = demandProfiles_.find(segment);
  return it == demandProfiles_.end() ? nullptr : &it->second;
}

const SegmentDemandProfile &MarketDemandSystem::segmentDemandProfile(
    MarketSegment segment) const {
  const auto *profile = profileFor(segment);
  if (!profile)
    throw std::invalid_argument("market segment demand profile missing");
  return *profile;
}

void MarketDemandSystem::setPlayerConsiderationBasisPoints(MarketSegment segment,
                                                           int basisPoints) {
  if (basisPoints < 0 || basisPoints > 10000)
    throw std::invalid_argument("invalid player consideration probability");
  playerConsiderationBasisPoints_[segment] = basisPoints;
}

bool MarketDemandSystem::playerConsidered(const BookingRequest &request) const {
  const auto it = playerConsiderationBasisPoints_.find(request.segment);
  const int basisPoints = it == playerConsiderationBasisPoints_.end()
                              ? kDefaultPlayerConsiderationBasisPoints
                              : it->second;
  if (basisPoints <= 0)
    return false;
  if (basisPoints >= 10000)
    return true;
  const std::uint64_t salt =
      static_cast<std::uint64_t>(static_cast<int>(request.segment) + 1) *
      0x9E3779B97F4A7C15ULL;
  const auto roll = mix64(seed_ ^ (request.id * 0xD1B54A32D192ED03ULL) ^ salt) % 10000ULL;
  return roll < static_cast<std::uint64_t>(basisPoints);
}

double MarketDemandSystem::potentialDemand(
    MarketSegment segment, int stayDay, const MarketDemandModifiers &modifiers) const {
  if (stayDay < 0 || !validBasisPoints(modifiers.seasonMultiplierBasisPoints) ||
      !validBasisPoints(modifiers.economicMultiplierBasisPoints) ||
      !validBasisPoints(modifiers.eventMultiplierBasisPoints) ||
      !validBasisPoints(modifiers.scenarioMultiplierBasisPoints))
    throw std::invalid_argument("invalid potential demand input");
  const auto *profile = profileFor(segment);
  if (!profile)
    return 0.0;
  const int weekday = stayDay % 7;
  double result = profile->baseDailyDemand;
  result *= static_cast<double>(profile->weekdayMultiplierBasisPoints[weekday]) / 10000.0;
  result *= static_cast<double>(modifiers.seasonMultiplierBasisPoints) / 10000.0;
  result *= static_cast<double>(modifiers.economicMultiplierBasisPoints) / 10000.0;
  result *= static_cast<double>(modifiers.eventMultiplierBasisPoints) / 10000.0;
  result *= static_cast<double>(modifiers.scenarioMultiplierBasisPoints) / 10000.0;
  return result;
}

int MarketDemandSystem::generatePotentialRequests(
    MarketSegment segment, int stayDay, const MarketDemandModifiers &modifiers) {
  const auto *profile = profileFor(segment);
  if (!profile)
    return 0;
  const double potential = potentialDemand(segment, stayDay, modifiers);
  if (potential <= 0.0)
    return 0;
  if (potential > static_cast<double>(std::numeric_limits<int>::max()))
    throw std::overflow_error("potential market demand too large");
  const int count = static_cast<int>(std::llround(potential));
  for (int i = 0; i < count; ++i) {
    BookingRequest request;
    request.id = nextRequestId_++;
    request.segment = segment;
    request.arrivalDay = stayDay;
    request.departureDay = stayDay + profile->medianStayNights;
    request.budgetCents = profile->baseBudgetCents;
    request.partySize = defaultPartySize(segment);
    request.amenityPreference = static_cast<int>(nextRandom() % 101);
    request.locationPreference = static_cast<int>(nextRandom() % 101);
    request.brandPreference = static_cast<int>(nextRandom() % 101);
    request.bookingDay = std::max(0, stayDay - profile->medianLeadTimeDays);
    snapshot_.requests.push_back(request);
    ++snapshot_.generatedRequests;
    allocate(request);
  }
  return count;
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

double MarketDemandSystem::reputationUtility(
    const BookingRequest &request, const MarketHotelOffer &hotel) const {
  const int overall = std::clamp(hotel.reputation, 0, 100);
  if (request.segment != MarketSegment::Business)
    return static_cast<double>(overall) / 100.0;

  const auto &c = hotel.reputationCategories;
  const int weighted =
      categoryOrOverall(c.service, overall) * 25 +
      categoryOrOverall(c.room, overall) * 20 +
      categoryOrOverall(c.cleanliness, overall) * 20 +
      categoryOrOverall(c.quiet, overall) * 15 +
      categoryOrOverall(c.business, overall) * 15 +
      categoryOrOverall(c.food, overall) * 5;
  return static_cast<double>(weighted) / 10000.0;
}

double MarketDemandSystem::playerChoiceWeight(const BookingRequest &request,
                                               const MarketHotelOffer &hotel) const {
  if (!isEligible(request, hotel))
    return 0.0;
  const double basePriceUtility = priceUtility(request, hotel);
  if (basePriceUtility <= 0.0)
    return 0.0;
  const auto *profile = profileFor(request.segment);
  const double elasticity = profile
                                ? static_cast<double>(profile->priceElasticityBasisPoints) /
                                      10000.0
                                : segmentPriceElasticity(request.segment);
  const double amenitySensitivity =
      profile ? static_cast<double>(profile->amenitySensitivityBasisPoints) / 10000.0
              : segmentAmenitySensitivity(request.segment);
  const SegmentChoiceWeights weights =
      profile ? profile->choiceWeights : SegmentChoiceWeights{};
  const double price = std::pow(basePriceUtility, elasticity);
  const double reputation = reputationUtility(request, hotel);
  const double amenities =
      (std::clamp(hotel.amenityScore, 0, 100) / 100.0) * amenitySensitivity;
  const double location = std::clamp(hotel.locationScore, 0, 100) / 100.0;
  const double stars = std::clamp(hotel.stars, 1, 5) / 5.0;
  const double brand = std::clamp(hotel.brandScore, 0, 100) / 100.0;
  const double roomFit = request.partySize <= 4 ? 1.0 : 0.85;

  const double score =
      static_cast<double>(weights.priceBasisPoints) / 10000.0 * price +
      static_cast<double>(weights.reputationBasisPoints) / 10000.0 * reputation +
      static_cast<double>(weights.amenityBasisPoints) / 10000.0 * amenities +
      static_cast<double>(weights.locationBasisPoints) / 10000.0 * location +
      static_cast<double>(weights.starBasisPoints) / 10000.0 * stars +
      static_cast<double>(weights.brandBasisPoints) / 10000.0 * brand +
      static_cast<double>(weights.roomBasisPoints) / 10000.0 * roomFit;
  const double temperature =
      static_cast<double>(std::max(1, choiceTemperatureBasisPoints_)) / 10000.0;
  return std::exp(std::clamp(score / temperature, -20.0, 20.0));
}

MarketHotelOffer MarketDemandSystem::effectiveCompetitorOffer(
    std::uint64_t hotelId, const BookingRequest &request) const {
  const auto it = std::find_if(competitors_.begin(), competitors_.end(),
                               [&](const auto &competitor) {
                                 return competitor.hotelId == hotelId;
                               });
  if (it == competitors_.end())
    return {};
  const auto &competitor = *it;
  MarketHotelOffer offer{competitor.hotelId, competitor.nightlyRateCents,
                         competitor.reputation, competitor.stars,
                         competitor.amenityScore, competitor.locationScore,
                         competitor.brandScore, true};
  offer.reputationCategories = competitor.reputationCategories;
  if (!competitor.roomCategories.empty() &&
      std::find(competitor.roomCategories.begin(), competitor.roomCategories.end(),
                request.roomCategory) == competitor.roomCategories.end()) {
    offer.sellable = false;
    return offer;
  }
  const auto window = std::find_if(
      competitor.inventoryWindows.begin(), competitor.inventoryWindows.end(),
      [&](const auto &candidate) {
        return candidate.roomCategory == request.roomCategory &&
               request.arrivalDay >= candidate.startDay &&
               request.arrivalDay <= candidate.endDay;
      });
  if (window != competitor.inventoryWindows.end()) {
    offer.nightlyRateCents = window->nightlyRateCents;
    offer.sellable = window->availableRooms > 0;
  }
  return offer;
}

void MarketDemandSystem::allocate(const BookingRequest &request) {
  struct Candidate {
    std::uint64_t id{};
    double weight{};
    bool player{};
  };
  std::vector<Candidate> candidates;
  const double playerWeight = playerChoiceWeight(request, player_);
  if (playerConsidered(request) && playerWeight > 0.0)
    candidates.push_back({player_.hotelId, playerWeight, true});
  for (const auto &competitor : competitors_) {
    const auto offer = effectiveCompetitorOffer(competitor.hotelId, request);
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
  const int daySpan = lastArrivalDay - firstArrivalDay + 1;
  for (int i = 0; i < count; ++i) {
    BookingRequest request;
    request.id = nextRequestId_++;
    request.segment = kSegments[nextRandom() % 9];
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
    if (const auto *profile = profileFor(request.segment))
      request.bookingDay = std::max(0, request.arrivalDay - profile->medianLeadTimeDays);
    else
      request.bookingDay = request.arrivalDay;
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
  snapshot_.comparableMedianRateCents =
      rates.size() % 2 ? rates[middle] : (rates[middle - 1] + rates[middle]) / 2;
}

MarketSnapshot MarketDemandSystem::snapshot() const {
  auto result = snapshot_;
  result.competitors = competitors_;
  result.physicalCompetitorGuests = 0;
  return result;
}

} // namespace hh::game
