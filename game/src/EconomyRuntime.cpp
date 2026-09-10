#include "hh/game/EconomyRuntime.h"

#include <algorithm>
#include <iomanip>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>

namespace hh::game {
namespace {

constexpr MarketSegment kSegments[] = {
    MarketSegment::BudgetLeisure, MarketSegment::Business,
    MarketSegment::ExecutiveBusiness, MarketSegment::CoupleLeisure,
    MarketSegment::FamilyLeisure, MarketSegment::LuxuryLeisure,
    MarketSegment::ConferenceGroup, MarketSegment::AirportTransit,
    MarketSegment::Wellness};
constexpr int kBasePlayerConsiderationBasisPoints = 7000;

BookingChannel channelFor(std::uint64_t requestId) {
  switch (requestId % 5) {
  case 0: return BookingChannel::Direct;
  case 1: return BookingChannel::Ota;
  case 2: return BookingChannel::Gds;
  case 3: return BookingChannel::Corporate;
  default: return BookingChannel::Group;
  }
}

bool validDemandMultiplier(int value) {
  return value >= 0 && value <= 100000;
}

} // namespace

EconomyRuntime::EconomyRuntime(std::uint64_t seed, std::int64_t openingCashCents)
    : seed_(seed ? seed : 1), market_(seed_), inventory_(seed_),
      economics_(openingCashCents) {
  inventory_.setCancellationBasisPoints(BookingChannel::Direct, 500);
  inventory_.setCancellationBasisPoints(BookingChannel::Ota, 1600);
  inventory_.setCancellationBasisPoints(BookingChannel::Gds, 900);
  inventory_.setCancellationBasisPoints(BookingChannel::Corporate, 600);
  inventory_.setCancellationBasisPoints(BookingChannel::Group, 1000);
  inventory_.setNoShowBasisPoints(BookingChannel::Direct, 300);
  inventory_.setNoShowBasisPoints(BookingChannel::Ota, 600);
  inventory_.setNoShowBasisPoints(BookingChannel::Gds, 400);
  inventory_.setNoShowBasisPoints(BookingChannel::Corporate, 200);
  inventory_.setNoShowBasisPoints(BookingChannel::Group, 300);
}

void EconomyRuntime::setPhysicalRoomCapacity(std::string category, int units) {
  if (category.empty() || units < 0)
    throw std::invalid_argument("invalid physical room capacity");
  physicalCapacity_[category] = units;
  inventory_.setPhysicalCapacity(category, units);
  inventory_.clearOverbookingAllowances(category);
  const auto policy = overbooking_.policies().find(category);
  if (policy != overbooking_.policies().end()) {
    if (policy->second.startDay == 0 &&
        policy->second.endDay == std::numeric_limits<int>::max())
      inventory_.setOverbookingAllowance(category, policy->second.allowance);
    else
      inventory_.setOverbookingAllowance(category, policy->second.allowance,
                                         policy->second.startDay,
                                         policy->second.endDay);
  }
}

void EconomyRuntime::setPlayerHotelOffer(const MarketHotelOffer &offer) {
  if (offer.hotelId == 0 || offer.nightlyRateCents <= 0)
    throw std::invalid_argument("invalid player hotel offer");
  basePlayerOffer_ = offer;
  market_.setPlayerOffer(offer);
  commercial_.setReputationProfile(std::clamp(offer.reputation, 0, 100) * 100,
                                   offer.reputationCategories);
}

void EconomyRuntime::setCompetitors(std::vector<CompetitorOffer> competitors) {
  market_.setCompetitors(std::move(competitors));
  const auto median = market_.snapshot().comparableMedianRateCents;
  if (median > 0 && !physicalCapacity_.empty())
    for (const auto &[category, units] : physicalCapacity_)
      if (units > 0)
        revenueManagement_.setComparableMedianCents(category, median);
}

void EconomyRuntime::setDemandModifiers(const MarketDemandModifiers &modifiers) {
  if (!validDemandMultiplier(modifiers.seasonMultiplierBasisPoints) ||
      !validDemandMultiplier(modifiers.economicMultiplierBasisPoints) ||
      !validDemandMultiplier(modifiers.eventMultiplierBasisPoints) ||
      !validDemandMultiplier(modifiers.scenarioMultiplierBasisPoints))
    throw std::invalid_argument("invalid market demand modifiers");
  demandModifiers_ = modifiers;
}

PricingRuleResult EconomyRuntime::setPricingRule(const PricingRuleCommand &command) {
  return revenueManagement_.setRule(command);
}

OverbookingResult EconomyRuntime::setOverbookingPolicy(const OverbookingPolicy &policy) {
  const auto result = overbooking_.setPolicy(policy);
  if (result.ok && physicalCapacity_.contains(policy.roomCategory)) {
    inventory_.clearOverbookingAllowances(policy.roomCategory);
    if (policy.startDay == 0 && policy.endDay == std::numeric_limits<int>::max())
      inventory_.setOverbookingAllowance(policy.roomCategory, policy.allowance);
    else
      inventory_.setOverbookingAllowance(policy.roomCategory, policy.allowance,
                                         policy.startDay, policy.endDay);
  }
  return result;
}

LoanResult EconomyRuntime::acceptLoan(const LoanOffer &offer) {
  const auto result = financing_.acceptLoan(offer, currentCashCents());
  if (!result.ok)
    return result;
  post(EconomicCategory::LoanProceeds, offer.principalCents, offer.id, "loan proceeds");
  if (offer.originationFeeCents > 0)
    post(EconomicCategory::LoanOriginationFee, -offer.originationFeeCents, offer.id,
         "loan origination fee");
  return result;
}

CommercialCommandResult EconomyRuntime::startMarketingCampaign(
    const MarketingCampaign &campaign) {
  const auto result = commercial_.startCampaign(campaign, currentDay_);
  if (result.ok && campaign.costCents > 0)
    post(EconomicCategory::MarketingCost, -campaign.costCents, campaign.id,
         "marketing campaign");
  return result;
}

ContractAcceptanceResult EconomyRuntime::acceptCommercialContract(
    const CommercialContract &contract, const ContractFeasibility &feasibility,
    bool acceptRisk) {
  const std::string category = physicalCapacity_.contains("standard")
                                   ? std::string("standard")
                                   : (physicalCapacity_.empty()
                                          ? std::string()
                                          : physicalCapacity_.begin()->first);
  std::int64_t actualAvailableRoomNights = 0;
  if (!category.empty() && contract.startDay >= 0 && contract.endDay >= contract.startDay) {
    for (int day = contract.startDay; day <= contract.endDay; ++day) {
      actualAvailableRoomNights += inventory_.availableUnits(day, category);
      if (day == std::numeric_limits<int>::max())
        break;
    }
  }
  const int boundedActual = static_cast<int>(std::min<std::int64_t>(
      actualAvailableRoomNights, std::numeric_limits<int>::max()));
  ContractFeasibility authoritative = feasibility;
  authoritative.availableRoomNights =
      std::min(feasibility.availableRoomNights, boundedActual);
  const auto accepted = commercial_.acceptContract(contract, authoritative, acceptRisk);
  if (!accepted.ok)
    return accepted;

  int reserved = 0;
  if (!category.empty()) {
    for (int day = contract.startDay;
         day <= contract.endDay && reserved < contract.minimumRoomNights; ++day) {
      int units = inventory_.availableUnits(day, category);
      while (units-- > 0 && reserved < contract.minimumRoomNights) {
        BookingRequestInput booking;
        booking.bookingId = nextBookingId_++;
        booking.arrivalDay = day;
        booking.departureDay = day + 1;
        booking.roomCategory = category;
        booking.rateCents = contract.negotiatedRateCents;
        booking.channel = BookingChannel::Group;
        booking.sourceContractId = contract.id;
        booking.paymentDelayDays = contract.paymentDelayDays;
        const auto booked = inventory_.book(booking);
        if (!booked.ok) {
          if (!acceptRisk)
            throw std::logic_error("prevalidated commercial contract booking failed");
          break;
        }
        ++reserved;
      }
      if (day == std::numeric_limits<int>::max())
        break;
    }
  }
  const int unfulfilled = std::max(0, contract.minimumRoomNights - reserved);
  if (unfulfilled > 0 && !acceptRisk)
    throw std::logic_error("feasible commercial contract did not reserve minimum room nights");
  commercial_.setContractCommitment(contract.id, reserved, unfulfilled);
  return {true, unfulfilled > 0 ? "ACCEPTED_WITH_EXPLICIT_RISK" : "OK",
          contract.id, unfulfilled > 0};
}

void EconomyRuntime::applyReviewOutcome(const ReviewSignal &review) {
  commercial_.applyReview(review);
}

int EconomyRuntime::physicalCapacityTotal() const {
  int total = 0;
  for (const auto &[category, units] : physicalCapacity_) {
    (void)category;
    total += units;
  }
  return total;
}

std::int64_t EconomyRuntime::currentCashCents() const {
  return economics_.snapshot(currentDay_, cumulativeSellableRoomNights_,
                             cumulativeOccupiedRoomNights_)
      .cashCents;
}

void EconomyRuntime::post(EconomicCategory category, std::int64_t amountCents,
                          std::uint64_t sourceId, std::string memo) {
  if (amountCents == 0)
    return;
  economics_.post({nextTransactionId_++, currentDay_, category, amountCents, sourceId,
                   std::move(memo)});
}

void EconomyRuntime::runOneDay() {
  const int capacity = physicalCapacityTotal();
  if (basePlayerOffer_.hotelId != 0) {
    auto offer = basePlayerOffer_;
    const auto commercial = commercial_.snapshot();
    offer.reputation = commercial.overallReputationBasisPoints / 100;
    offer.reputationCategories = {
        commercial.serviceReputationBasisPoints / 100,
        commercial.roomReputationBasisPoints / 100,
        commercial.cleanlinessReputationBasisPoints / 100,
        commercial.quietReputationBasisPoints / 100,
        commercial.businessReputationBasisPoints / 100,
        commercial.foodReputationBasisPoints / 100};
    const std::string pricingCategory =
        physicalCapacity_.contains("standard")
            ? std::string("standard")
            : (physicalCapacity_.empty() ? std::string() : physicalCapacity_.begin()->first);
    if (!pricingCategory.empty()) {
      const int sellable = inventory_.sellableUnits(currentDay_ + 7, pricingCategory);
      const int booked = inventory_.bookedUnits(currentDay_ + 7, pricingCategory);
      const int occupancyBp =
          sellable > 0 ? std::clamp(booked * 10000 / sellable, 0, 10000) : 0;
      offer.nightlyRateCents = revenueManagement_.effectiveRateCents(
          currentDay_, currentDay_ % 7, pricingCategory, occupancyBp,
          basePlayerOffer_.nightlyRateCents);
    }
    market_.setPlayerOffer(offer);
  }

  const auto beforeMarket = market_.snapshot();
  const std::size_t oldRequestCount = beforeMarket.requests.size();
  const std::size_t oldChoiceCount = beforeMarket.choices.size();
  for (const auto segment : kSegments) {
    const int visibility = commercial_.visibilityBasisPoints(segment, currentDay_);
    const auto consideration = static_cast<int>(std::clamp<std::int64_t>(
        static_cast<std::int64_t>(kBasePlayerConsiderationBasisPoints) * visibility /
            10000,
        0, 10000));
    market_.setPlayerConsiderationBasisPoints(segment, consideration);
    const auto &profile = market_.segmentDemandProfile(segment);
    const int stayDay = currentDay_ + profile.medianLeadTimeDays;
    (void)market_.generatePotentialRequests(segment, stayDay, demandModifiers_);
  }
  const auto afterMarket = market_.snapshot();

  if (!physicalCapacity_.empty()) {
    const std::string category = physicalCapacity_.contains("standard")
                                     ? std::string("standard")
                                     : physicalCapacity_.begin()->first;
    const std::size_t newCount = std::min(afterMarket.requests.size() - oldRequestCount,
                                          afterMarket.choices.size() - oldChoiceCount);
    for (std::size_t offset = 0; offset < newCount; ++offset) {
      const auto &request = afterMarket.requests[oldRequestCount + offset];
      const auto &choice = afterMarket.choices[oldChoiceCount + offset];
      if (!choice.playerWon)
        continue;
      const auto pricing = revenueManagement_.snapshot();
      const auto rateIt = pricing.effectiveRateCents.find(category);
      const auto rate = rateIt == pricing.effectiveRateCents.end()
                            ? basePlayerOffer_.nightlyRateCents
                            : rateIt->second;
      BookingRequestInput booking;
      booking.bookingId = nextBookingId_++;
      booking.arrivalDay = request.arrivalDay;
      booking.departureDay = request.departureDay;
      booking.roomCategory = category;
      booking.rateCents = rate;
      booking.channel = channelFor(request.id);
      (void)inventory_.book(booking);
    }
  }

  const auto paymentInventory = inventory_.snapshot();
  for (const auto &booking : paymentInventory.bookings) {
    const bool payableState = booking.state == BookingState::Confirmed ||
                              booking.state == BookingState::Completed;
    if (!payableState || booking.revenuePosted || booking.paymentDay != currentDay_)
      continue;
    const auto nights = static_cast<std::int64_t>(booking.departureDay - booking.arrivalDay);
    post(EconomicCategory::RoomRevenue, booking.rateCents * nights, booking.bookingId,
         booking.sourceContractId == 0 ? "completed room stay"
                                       : "commercial contract room revenue");
    if (booking.commissionCents > 0)
      post(EconomicCategory::ChannelCommissionCost, -booking.commissionCents,
           booking.bookingId, "booking channel commission");
    inventory_.markRevenuePosted(booking.bookingId);
  }

  const auto beforeInventory = inventory_.snapshot();
  std::map<std::uint64_t, BookingState> stateBefore;
  for (const auto &booking : beforeInventory.bookings)
    stateBefore[booking.bookingId] = booking.state;
  inventory_.processDay(currentDay_);
  const auto afterInventory = inventory_.snapshot();
  for (const auto &booking : afterInventory.bookings) {
    const auto previous = stateBefore.find(booking.bookingId);
    if (previous == stateBefore.end() || previous->second != BookingState::Confirmed)
      continue;
    if (booking.state == BookingState::Cancelled) {
      if (booking.cancellationPenaltyCents > 0)
        post(EconomicCategory::CancellationFeeRevenue,
             booking.cancellationPenaltyCents, booking.bookingId,
             "cancellation fee");
    } else if (booking.state == BookingState::NoShow) {
      post(EconomicCategory::NoShowFeeRevenue, booking.rateCents, booking.bookingId,
           "no-show fee");
    }
  }

  int occupiedToday = 0;
  for (const auto &booking : afterInventory.bookings)
    if (booking.state == BookingState::Confirmed && booking.arrivalDay <= currentDay_ &&
        currentDay_ < booking.departureDay)
      ++occupiedToday;
  cumulativeSellableRoomNights_ += capacity;
  cumulativeOccupiedRoomNights_ += std::min(capacity, occupiedToday);

  if (capacity > 0)
    post(EconomicCategory::UtilityCost, -static_cast<std::int64_t>(capacity) * 350, 0,
         "daily room utilities");
  post(EconomicCategory::FixedPeriodicExpense, -5000, 0, "daily fixed operating expense");

  const auto debt = financing_.processDay(currentDay_, currentCashCents());
  if (debt.debtServiceCents > 0)
    post(EconomicCategory::DebtService, -debt.debtServiceCents, 0, "scheduled debt service");
  const auto averageDailyCost = static_cast<std::int64_t>(capacity) * 350 + 5000;
  const auto ledgerSnapshot = economics_.snapshot(
      currentDay_, cumulativeSellableRoomNights_, cumulativeOccupiedRoomNights_);
  financing_.observeDay(currentDay_, currentCashCents(), averageDailyCost,
                        debt.missedObligationCents > 0, ledgerSnapshot.gopCents);

  ++currentDay_;
}

void EconomyRuntime::runDays(int days) {
  if (days < 0)
    throw std::invalid_argument("negative campaign duration");
  for (int i = 0; i < days; ++i)
    runOneDay();
}

MarketSnapshot EconomyRuntime::marketSnapshot() const { return market_.snapshot(); }

RevenueManagementSnapshot EconomyRuntime::revenueManagementSnapshot() const {
  auto out = revenueManagement_.snapshot();
  out.inventory = inventory_.snapshot();
  return out;
}

FinancialSnapshot EconomyRuntime::financialSnapshot() const {
  return {economics_.snapshot(currentDay_, cumulativeSellableRoomNights_,
                              cumulativeOccupiedRoomNights_),
          financing_.snapshot()};
}

CommercialDemandSnapshot EconomyRuntime::commercialSnapshot() const {
  return commercial_.snapshot();
}

int EconomyRuntime::currentDay() const noexcept { return currentDay_; }

std::string EconomyRuntime::save() const {
  std::ostringstream out;
  out << "HHECONRT 2 " << seed_ << ' ' << currentDay_ << ' ' << nextBookingId_ << ' '
      << nextTransactionId_ << ' ' << cumulativeSellableRoomNights_ << ' '
      << cumulativeOccupiedRoomNights_ << ' ' << basePlayerOffer_.hotelId << ' '
      << basePlayerOffer_.nightlyRateCents << ' ' << basePlayerOffer_.reputation << ' '
      << basePlayerOffer_.stars << ' ' << basePlayerOffer_.amenityScore << ' '
      << basePlayerOffer_.locationScore << ' ' << basePlayerOffer_.brandScore << ' '
      << basePlayerOffer_.sellable << ' ' << demandModifiers_.seasonMultiplierBasisPoints << ' '
      << demandModifiers_.economicMultiplierBasisPoints << ' '
      << demandModifiers_.eventMultiplierBasisPoints << ' '
      << demandModifiers_.scenarioMultiplierBasisPoints << ' ' << physicalCapacity_.size();
  for (const auto &[category, units] : physicalCapacity_)
    out << ' ' << std::quoted(category) << ' ' << units;
  out << ' ' << std::quoted(market_.save()) << ' ' << std::quoted(inventory_.save()) << ' '
      << std::quoted(revenueManagement_.save()) << ' ' << std::quoted(economics_.save()) << ' '
      << std::quoted(financing_.save()) << ' ' << std::quoted(overbooking_.save()) << ' '
      << std::quoted(commercial_.save());
  return out.str();
}

EconomyRuntime EconomyRuntime::load(std::string_view data) {
  if (data.size() > 64 * 1024 * 1024)
    throw std::invalid_argument("economy runtime save too large");
  std::istringstream in{std::string(data)};
  std::string magic;
  int version{};
  std::uint64_t seed{};
  std::int64_t sellable{}, occupied{};
  in >> magic >> version >> seed;
  if (!in || magic != "HHECONRT" || (version != 1 && version != 2) || seed == 0)
    throw std::invalid_argument("invalid economy runtime save");
  EconomyRuntime result(seed, 0);
  in >> result.currentDay_ >> result.nextBookingId_ >> result.nextTransactionId_ >>
      sellable >> occupied >> result.basePlayerOffer_.hotelId >>
      result.basePlayerOffer_.nightlyRateCents >> result.basePlayerOffer_.reputation >>
      result.basePlayerOffer_.stars >> result.basePlayerOffer_.amenityScore >>
      result.basePlayerOffer_.locationScore >> result.basePlayerOffer_.brandScore >>
      result.basePlayerOffer_.sellable;
  if (version >= 2)
    in >> result.demandModifiers_.seasonMultiplierBasisPoints >>
        result.demandModifiers_.economicMultiplierBasisPoints >>
        result.demandModifiers_.eventMultiplierBasisPoints >>
        result.demandModifiers_.scenarioMultiplierBasisPoints;
  result.cumulativeSellableRoomNights_ = sellable;
  result.cumulativeOccupiedRoomNights_ = occupied;
  std::size_t count{};
  in >> count;
  if (!in || result.currentDay_ < 0 || result.nextBookingId_ == 0 ||
      result.nextTransactionId_ == 0 || sellable < 0 || occupied < 0 || occupied > sellable ||
      count > 10000 || !validDemandMultiplier(result.demandModifiers_.seasonMultiplierBasisPoints) ||
      !validDemandMultiplier(result.demandModifiers_.economicMultiplierBasisPoints) ||
      !validDemandMultiplier(result.demandModifiers_.eventMultiplierBasisPoints) ||
      !validDemandMultiplier(result.demandModifiers_.scenarioMultiplierBasisPoints))
    throw std::invalid_argument("invalid economy runtime state");
  result.physicalCapacity_.clear();
  for (std::size_t i = 0; i < count; ++i) {
    std::string category;
    int units{};
    in >> std::quoted(category) >> units;
    if (!in || category.empty() || units < 0)
      throw std::invalid_argument("invalid saved room capacity");
    result.physicalCapacity_[std::move(category)] = units;
  }
  std::string marketSave, inventorySave, rmSave, economicsSave, financingSave,
      overbookingSave, commercialSave;
  in >> std::quoted(marketSave) >> std::quoted(inventorySave) >> std::quoted(rmSave) >>
      std::quoted(economicsSave) >> std::quoted(financingSave) >>
      std::quoted(overbookingSave) >> std::quoted(commercialSave);
  if (!in)
    throw std::invalid_argument("incomplete economy runtime save");
  result.market_ = MarketDemandSystem::load(marketSave);
  result.inventory_ = RevenueInventory::load(inventorySave);
  result.revenueManagement_ = RevenueManagement::load(rmSave);
  result.economics_ = HotelEconomics::load(economicsSave);
  result.financing_ = FinancingSystem::load(financingSave);
  result.overbooking_ = OverbookingSystem::load(overbookingSave);
  result.commercial_ = CommercialDemand::load(commercialSave);
  in >> std::ws;
  if (!in.eof())
    throw std::invalid_argument("unexpected economy runtime trailing data");
  return result;
}

std::uint64_t EconomyRuntime::authoritativeHash() const {
  const auto encoded = save();
  std::uint64_t hash = 1469598103934665603ULL;
  for (const unsigned char byte : encoded) {
    hash ^= byte;
    hash *= 1099511628211ULL;
  }
  return hash;
}

std::size_t EconomyRuntime::estimatedStateBytes() const { return save().size(); }

} // namespace hh::game
