#include "hh/game/SimulationEconomyBridge.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace hh::game {
namespace {

std::uint64_t fnv1a(std::string_view text) {
  std::uint64_t hash = 1469598103934665603ULL;
  for (const unsigned char c : text) {
    hash ^= c;
    hash *= 1099511628211ULL;
  }
  return hash;
}

} // namespace

SimulationEconomyBridge::SimulationEconomyBridge(std::uint64_t seed)
    : simulation_(seed),
      economy_(seed, simulation_.view().economy.cashCents) {
  resetBaseline();
  synchronizeCapacity();
  economy_.synchronizeExternalMetrics(simulation_.view().day, 0, 0);
}

SimulationEconomyBridge SimulationEconomyBridge::tutorial(std::uint64_t seed) {
  SimulationEconomyBridge result(seed);
  result.simulation_ = Simulation::tutorial(seed);
  result.economy_ =
      EconomyRuntime(seed, result.simulation_.view().economy.cashCents);
  result.cumulativeSellableRoomNights_ = 0;
  result.cumulativeOccupiedRoomNights_ = 0;
  result.resetBaseline();
  result.synchronizeCapacity();
  result.economy_.synchronizeExternalMetrics(result.simulation_.view().day, 0, 0);
  return result;
}

void SimulationEconomyBridge::resetBaseline() {
  lastEconomy_ = simulation_.view().economy;
  lastFoodRevenueCents_ = simulation_.foodServiceSnapshot().revenueCents;
  lastEventRevenueCents_ = simulation_.eventsSnapshot().revenueCents;
  lastAmenityRevenueCents_ = simulation_.amenitiesSnapshot().revenueCents;
}

void SimulationEconomyBridge::synchronizeCapacity() {
  const auto current = simulation_.view();
  economy_.setPhysicalRoomCapacity("standard",
                                   static_cast<int>(current.rooms.size()));
}

void SimulationEconomyBridge::reconcile(std::uint64_t sourceId,
                                        std::string_view memo) {
  const auto currentView = simulation_.view();
  const auto &current = currentView.economy;
  const auto foodRevenue = simulation_.foodServiceSnapshot().revenueCents;
  const auto eventRevenue = simulation_.eventsSnapshot().revenueCents;
  const auto amenityRevenue = simulation_.amenitiesSnapshot().revenueCents;

  const auto totalRevenueDelta = current.revenueCents - lastEconomy_.revenueCents;
  const auto foodDelta = foodRevenue - lastFoodRevenueCents_;
  const auto eventDelta = eventRevenue - lastEventRevenueCents_;
  const auto amenityDelta = amenityRevenue - lastAmenityRevenueCents_;
  const auto roomDelta = totalRevenueDelta - foodDelta - eventDelta - amenityDelta;
  const auto payrollDelta = current.payrollCents - lastEconomy_.payrollCents;
  const auto supplyDelta = current.supplyCostCents - lastEconomy_.supplyCostCents;
  const auto constructionDelta =
      current.constructionCostCents - lastEconomy_.constructionCostCents;
  const auto utilityDelta = current.utilityCostCents - lastEconomy_.utilityCostCents;

  if (totalRevenueDelta < 0 || foodDelta < 0 || eventDelta < 0 ||
      amenityDelta < 0 || roomDelta < 0 || payrollDelta < 0 || supplyDelta < 0 ||
      constructionDelta < 0 || utilityDelta < 0)
    throw std::logic_error("Simulation economic counters moved backwards");

  std::int64_t postedCashDelta = 0;
  const auto postPositive = [&](EconomicCategory category, std::int64_t amount,
                                std::string suffix) {
    if (amount <= 0)
      return;
    economy_.postExternalTransaction(
        currentView.day, category, amount, sourceId,
        std::string(memo) + ": " + std::move(suffix));
    postedCashDelta += amount;
  };
  const auto postCost = [&](EconomicCategory category, std::int64_t amount,
                            std::string suffix) {
    if (amount <= 0)
      return;
    economy_.postExternalTransaction(
        currentView.day, category, -amount, sourceId,
        std::string(memo) + ": " + std::move(suffix));
    postedCashDelta -= amount;
  };

  postPositive(EconomicCategory::RoomRevenue, roomDelta, "room revenue");
  postPositive(EconomicCategory::RestaurantFoodRevenue, foodDelta,
               "food and beverage revenue");
  postPositive(EconomicCategory::EventRevenue, eventDelta, "event revenue");
  postPositive(EconomicCategory::PremiumServiceRevenue, amenityDelta,
               "amenity revenue");
  postCost(EconomicCategory::LaborCost, payrollDelta, "payroll");
  postCost(EconomicCategory::SupplyCost, supplyDelta, "supplies");
  postCost(EconomicCategory::CapitalExpense, constructionDelta,
           "construction");
  postCost(EconomicCategory::UtilityCost, utilityDelta, "utilities");

  const auto legacyCashDelta = current.cashCents - lastEconomy_.cashCents;
  if (postedCashDelta != legacyCashDelta)
    throw std::logic_error("unreconciled Simulation cash movement");

  lastEconomy_ = current;
  lastFoodRevenueCents_ = foodRevenue;
  lastEventRevenueCents_ = eventRevenue;
  lastAmenityRevenueCents_ = amenityRevenue;

  const auto financial = economy_.financialSnapshot().economics;
  if (financial.cashCents != current.cashCents || !financial.reconciled)
    throw std::logic_error("FINAL-06 ledger diverged from Simulation cash");
}

void SimulationEconomyBridge::recordDayBoundary(const SimulationView &current) {
  std::int64_t sellable = 0;
  std::int64_t occupied = 0;
  for (const auto &room : current.rooms) {
    if (room.closed || room.status == RoomStatus::Incomplete ||
        room.status == RoomStatus::OutOfOrder)
      continue;
    ++sellable;
    if (room.status == RoomStatus::Occupied)
      ++occupied;
  }
  cumulativeSellableRoomNights_ += sellable;
  cumulativeOccupiedRoomNights_ += occupied;
}

CommandResult SimulationEconomyBridge::buildTile(Position position, TileKind kind) {
  const auto result = simulation_.buildTile(position, kind);
  reconcile(result.id, "build tile");
  return result;
}

CommandResult SimulationEconomyBridge::buildFurnishedRoom(
    const RoomBlueprint &blueprint) {
  const auto result = simulation_.buildFurnishedRoom(blueprint);
  reconcile(result.id, "build furnished room");
  if (result.ok)
    synchronizeCapacity();
  return result;
}

CommandResult SimulationEconomyBridge::hireStaff(const StaffHire &hire) {
  const auto result = simulation_.hireStaff(hire);
  reconcile(result.id, "hire staff");
  return result;
}

CommandResult SimulationEconomyBridge::fireStaff(EntityId employeeId) {
  const auto result = simulation_.fireStaff(employeeId);
  reconcile(employeeId, "fire staff");
  return result;
}

CommandResult SimulationEconomyBridge::setStaffShift(EntityId employeeId,
                                                      int startHour,
                                                      int endHour) {
  const auto result = simulation_.setStaffShift(employeeId, startHour, endHour);
  reconcile(employeeId, "set staff shift");
  return result;
}

CommandResult SimulationEconomyBridge::setRoomRate(EntityId roomId, double rate) {
  const auto result = simulation_.setRoomRate(roomId, rate);
  reconcile(roomId, "set room rate");
  return result;
}

CommandResult SimulationEconomyBridge::requestClean(EntityId roomId) {
  const auto result = simulation_.requestClean(roomId);
  reconcile(roomId, "request clean");
  return result;
}

CommandResult SimulationEconomyBridge::requestRepair(EntityId roomId) {
  const auto result = simulation_.requestRepair(roomId);
  reconcile(roomId, "request repair");
  return result;
}

CommandResult SimulationEconomyBridge::closeRoom(EntityId roomId, bool closed) {
  const auto result = simulation_.closeRoom(roomId, closed);
  reconcile(roomId, closed ? "close room" : "reopen room");
  return result;
}

CommandResult SimulationEconomyBridge::removeRoom(EntityId roomId) {
  const auto result = simulation_.removeRoom(roomId);
  reconcile(roomId, "remove room");
  if (result.ok)
    synchronizeCapacity();
  return result;
}

CommandResult SimulationEconomyBridge::orderSupplies(const SupplyOrder &order) {
  const auto result = simulation_.orderSupplies(order);
  reconcile(result.id, "order supplies");
  return result;
}

CommandResult SimulationEconomyBridge::loadDefinitions(std::string_view jsonText) {
  const auto result = simulation_.loadDefinitions(jsonText);
  reconcile(result.id, "load definitions");
  return result;
}

void SimulationEconomyBridge::step(double seconds) {
  if (!std::isfinite(seconds) || seconds < 0.0)
    throw std::invalid_argument("invalid bridge step");
  double remaining = seconds;
  while (remaining > 0.0) {
    const auto before = simulation_.view();
    const auto secondsIntoDay = before.elapsedSeconds % 86400;
    const double toBoundary =
        static_cast<double>(86400 - secondsIntoDay);
    const double chunk = std::min(remaining, toBoundary);
    simulation_.step(chunk);
    reconcile(0, "simulation step");
    const auto after = simulation_.view();
    if (after.day > before.day) {
      recordDayBoundary(after);
      synchronizeCapacity();
    }
    economy_.synchronizeExternalMetrics(after.day, cumulativeSellableRoomNights_,
                                        cumulativeOccupiedRoomNights_);
    remaining -= chunk;
    if (chunk <= 0.0)
      break;
  }
}

void SimulationEconomyBridge::setPlayerHotelOffer(
    const MarketHotelOffer &offer) {
  economy_.setPlayerHotelOffer(offer);
}

void SimulationEconomyBridge::setCompetitors(
    std::vector<CompetitorOffer> competitors) {
  economy_.setCompetitors(std::move(competitors));
}

PricingRuleResult SimulationEconomyBridge::setPricingRule(
    const PricingRuleCommand &command) {
  return economy_.setPricingRule(command);
}

OverbookingResult SimulationEconomyBridge::setOverbookingPolicy(
    const OverbookingPolicy &policy) {
  return economy_.setOverbookingPolicy(policy);
}

void SimulationEconomyBridge::applyReviewOutcome(const ReviewSignal &review) {
  economy_.applyReviewOutcome(review);
}

SimulationView SimulationEconomyBridge::view() const { return simulation_.view(); }
LogisticsSnapshot SimulationEconomyBridge::logisticsSnapshot() const {
  return simulation_.logisticsSnapshot();
}
FoodServiceSnapshot SimulationEconomyBridge::foodServiceSnapshot() const {
  return simulation_.foodServiceSnapshot();
}
EventsSnapshot SimulationEconomyBridge::eventsSnapshot() const {
  return simulation_.eventsSnapshot();
}
AmenitiesSnapshot SimulationEconomyBridge::amenitiesSnapshot() const {
  return simulation_.amenitiesSnapshot();
}
MarketSnapshot SimulationEconomyBridge::marketSnapshot() const {
  return economy_.marketSnapshot();
}
RevenueManagementSnapshot
SimulationEconomyBridge::revenueManagementSnapshot() const {
  return economy_.revenueManagementSnapshot();
}
FinancialSnapshot SimulationEconomyBridge::financialSnapshot() const {
  return economy_.financialSnapshot();
}
CommercialDemandSnapshot SimulationEconomyBridge::commercialSnapshot() const {
  return economy_.commercialSnapshot();
}

std::string SimulationEconomyBridge::save() const {
  std::ostringstream out;
  out << "HHSIMECO 1 " << cumulativeSellableRoomNights_ << ' '
      << cumulativeOccupiedRoomNights_ << ' ' << std::quoted(simulation_.save())
      << ' ' << std::quoted(economy_.save());
  return out.str();
}

SimulationEconomyBridge SimulationEconomyBridge::load(std::string_view data) {
  if (data.size() > 96 * 1024 * 1024)
    throw std::invalid_argument("integrated simulation economy save too large");
  std::istringstream in{std::string(data)};
  std::string magic;
  int version{};
  std::int64_t sellable{}, occupied{};
  std::string simulationState, economyState;
  in >> magic >> version >> sellable >> occupied >> std::quoted(simulationState) >>
      std::quoted(economyState);
  if (!in || magic != "HHSIMECO" || version != 1 || sellable < 0 || occupied < 0 ||
      occupied > sellable)
    throw std::invalid_argument("invalid integrated simulation economy save");
  in >> std::ws;
  if (!in.eof())
    throw std::invalid_argument("unexpected integrated save trailing data");

  SimulationEconomyBridge result(1);
  result.simulation_ = Simulation::load(simulationState);
  result.economy_ = EconomyRuntime::load(economyState);
  result.cumulativeSellableRoomNights_ = sellable;
  result.cumulativeOccupiedRoomNights_ = occupied;
  result.resetBaseline();

  const auto simulationCash = result.simulation_.view().economy.cashCents;
  const auto financial = result.economy_.financialSnapshot().economics;
  if (financial.cashCents != simulationCash || !financial.reconciled)
    throw std::invalid_argument("integrated save economic ledgers do not reconcile");
  return result;
}

std::uint64_t SimulationEconomyBridge::authoritativeHash() const {
  return fnv1a(save());
}

} // namespace hh::game
