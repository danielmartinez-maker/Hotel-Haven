#include "hh/game/EconomyRuntime.h"

#include <algorithm>
#include <iomanip>
#include <map>
#include <sstream>
#include <stdexcept>

namespace hh::game {
namespace {

BookingChannel channelFor(std::uint64_t requestId) {
  switch (requestId % 5) {
  case 0: return BookingChannel::Direct;
  case 1: return BookingChannel::Ota;
  case 2: return BookingChannel::Gds;
  case 3: return BookingChannel::Corporate;
  default: return BookingChannel::Group;
  }
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
  inventory_.setOverbookingAllowance(category, overbooking_.allowance(category));
}

void EconomyRuntime::setPlayerHotelOffer(const MarketHotelOffer &offer) {
  if (offer.hotelId == 0 || offer.nightlyRateCents <= 0)
    throw std::invalid_argument("invalid player hotel offer");
  basePlayerOffer_ = offer;
  market_.setPlayerOffer(offer);
  commercial_.setReputation(std::clamp(offer.reputation, 0, 100) * 100);
}

void EconomyRuntime::setCompetitors(std::vector<CompetitorOffer> competitors) {
  market_.setCompetitors(std::move(competitors));
  const auto median = market_.snapshot().comparableMedianRateCents;
  if (median > 0 && !physicalCapacity_.empty())
    for (const auto &[category, units] : physicalCapacity_)
      if (units > 0)
        revenueManagement_.setComparableMedianCents(category, median);
}

PricingRuleResult EconomyRuntime::setPricingRule(const PricingRuleCommand &command) {
  return revenueManagement_.setRule(command);
}

OverbookingResult EconomyRuntime::setOverbookingPolicy(const OverbookingPolicy &policy) {
  const auto result = overbooking_.setPolicy(policy);
  if (result.ok && physicalCapacity_.contains(policy.roomCategory))
    inventory_.setOverbookingAllowance(policy.roomCategory, policy.allowance);
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
  return commercial_.acceptContract(contract, feasibility, acceptRisk);
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
    offer.reputation = commercial_.snapshot().overallReputationBasisPoints / 100;
    std::string pricingCategory = physicalCapacity_.contains("standard")
                                      ? std::string("standard")
                                      : (physicalCapacity_.empty() ? std::string() : physicalCapacity_.begin()->first);
    if (!pricingCategory.empty()) {
      const int sellable = inventory_.sellableUnits(currentDay_ + 7, pricingCategory);
      const int booked = inventory_.bookedUnits(currentDay_ + 7, pricingCategory);
      const int occupancyBp = sellable > 0 ? std::clamp(booked * 10000 / sellable, 0, 10000) : 0;
      offer.nightlyRateCents = revenueManagement_.effectiveRateCents(
          currentDay_, currentDay_ % 7, pricingCategory, occupancyBp,
          basePlayerOffer_.nightlyRateCents);
    }
    market_.setPlayerOffer(offer);
  }

  const auto beforeMarket = market_.snapshot();
  const std::size_t oldRequestCount = beforeMarket.requests.size();
  const std::size_t oldChoiceCount = beforeMarket.choices.size();
  const int requestCount = std::max(4, capacity / 20);
  market_.generateRequests(currentDay_ + 1, currentDay_ + 14, requestCount);
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

  // Realized room revenue and commissions are posted at contractual departure.
  auto beforeInventory = inventory_.snapshot();
  for (const auto &booking : beforeInventory.bookings) {
    if (booking.state != BookingState::Confirmed || booking.departureDay != currentDay_)
      continue;
    const auto nights = static_cast<std::int64_t>(booking.departureDay - booking.arrivalDay);
    post(EconomicCategory::RoomRevenue, booking.rateCents * nights, booking.bookingId,
         "completed room stay");
    if (booking.commissionCents > 0)
      post(EconomicCategory::ChannelCommissionCost, -booking.commissionCents,
           booking.bookingId, "booking channel commission");
    inventory_.complete(booking.bookingId);
  }

  beforeInventory = inventory_.snapshot();
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
      const auto fee = std::max<std::int64_t>(1, booking.rateCents / 5);
      post(EconomicCategory::CancellationFeeRevenue, fee, booking.bookingId,
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
  financing_.observeDay(currentDay_, currentCashCents(), averageDailyCost,
                        debt.missedObligationCents > 0);

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
  out << "HHECONRT 1 " << seed_ << ' ' << currentDay_ << ' ' << nextBookingId_ << ' '
      << nextTransactionId_ << ' ' << cumulativeSellableRoomNights_ << ' '
      << cumulativeOccupiedRoomNights_ << ' ' << basePlayerOffer_.hotelId << ' '
      << basePlayerOffer_.nightlyRateCents << ' ' << basePlayerOffer_.reputation << ' '
      << basePlayerOffer_.stars << ' ' << basePlayerOffer_.amenityScore << ' '
      << basePlayerOffer_.locationScore << ' ' << basePlayerOffer_.brandScore << ' '
      << basePlayerOffer_.sellable << ' ' << physicalCapacity_.size();
  for (const auto &[category, units] : physicalCapacity_)
    out << ' ' << std::quoted(category) << ' ' << units;
  out << ' ' << std::quoted(market_.save())
      << ' ' << std::quoted(inventory_.save())
      << ' ' << std::quoted(revenueManagement_.save())
      << ' ' << std::quoted(economics_.save())
      << ' ' << std::quoted(financing_.save())
      << ' ' << std::quoted(overbooking_.save())
      << ' ' << std::quoted(commercial_.save());
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
  if (!in || magic != "HHECONRT" || version != 1 || seed == 0)
    throw std::invalid_argument("invalid economy runtime save");
  EconomyRuntime result(seed, 0);
  in >> result.currentDay_ >> result.nextBookingId_ >> result.nextTransactionId_ >>
      sellable >> occupied >> result.basePlayerOffer_.hotelId >>
      result.basePlayerOffer_.nightlyRateCents >> result.basePlayerOffer_.reputation >>
      result.basePlayerOffer_.stars >> result.basePlayerOffer_.amenityScore >>
      result.basePlayerOffer_.locationScore >> result.basePlayerOffer_.brandScore >>
      result.basePlayerOffer_.sellable;
  result.cumulativeSellableRoomNights_ = sellable;
  result.cumulativeOccupiedRoomNights_ = occupied;
  std::size_t count{};
  in >> count;
  if (!in || result.currentDay_ < 0 || result.nextBookingId_ == 0 ||
      result.nextTransactionId_ == 0 || sellable < 0 || occupied < 0 || occupied > sellable ||
      count > 10000)
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

std::size_t EconomyRuntime::estimatedStateBytes() const {
  return save().size();
}

} // namespace hh::game
