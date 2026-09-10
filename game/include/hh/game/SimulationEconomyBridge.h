#pragma once

#include "hh/game/EconomyRuntime.h"
#include "hh/game/Simulation.h"
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace hh::game {

// Owns the authoritative physical Simulation and the FINAL-06 economic ledger.
// The legacy Simulation remains the authority for physical guests, rooms,
// service work, and immediate cash mutations. The bridge mirrors every cash
// delta exactly once into EconomyRuntime and fails closed on any mismatch.
class SimulationEconomyBridge {
public:
  explicit SimulationEconomyBridge(std::uint64_t seed = 1);
  static SimulationEconomyBridge tutorial(std::uint64_t seed = 1);

  CommandResult buildTile(Position position, TileKind kind);
  CommandResult buildFurnishedRoom(const RoomBlueprint &blueprint);
  CommandResult hireStaff(const StaffHire &hire);
  CommandResult fireStaff(EntityId employeeId);
  CommandResult setStaffShift(EntityId employeeId, int startHour, int endHour);
  CommandResult setRoomRate(EntityId roomId, double rate);
  CommandResult requestClean(EntityId roomId);
  CommandResult requestRepair(EntityId roomId);
  CommandResult closeRoom(EntityId roomId, bool closed);
  CommandResult removeRoom(EntityId roomId);
  CommandResult orderSupplies(const SupplyOrder &order);
  CommandResult loadDefinitions(std::string_view jsonText);

  void step(double seconds);

  void setPlayerHotelOffer(const MarketHotelOffer &offer);
  void setCompetitors(std::vector<CompetitorOffer> competitors);
  [[nodiscard]] PricingRuleResult setPricingRule(const PricingRuleCommand &command);
  [[nodiscard]] OverbookingResult setOverbookingPolicy(const OverbookingPolicy &policy);
  void applyReviewOutcome(const ReviewSignal &review);

  [[nodiscard]] SimulationView view() const;
  [[nodiscard]] LogisticsSnapshot logisticsSnapshot() const;
  [[nodiscard]] FoodServiceSnapshot foodServiceSnapshot() const;
  [[nodiscard]] EventsSnapshot eventsSnapshot() const;
  [[nodiscard]] AmenitiesSnapshot amenitiesSnapshot() const;
  [[nodiscard]] MarketSnapshot marketSnapshot() const;
  [[nodiscard]] RevenueManagementSnapshot revenueManagementSnapshot() const;
  [[nodiscard]] FinancialSnapshot financialSnapshot() const;
  [[nodiscard]] CommercialDemandSnapshot commercialSnapshot() const;
  [[nodiscard]] EconomyDiagnostics economyDiagnostics() const {
    return economy_.diagnostics();
  }
  [[nodiscard]] const std::map<std::string, OverbookingPolicy> &
  overbookingPolicies() const noexcept { return economy_.overbookingPolicies(); }

  // Read-only application seams used by presentation adapters. Mutations still
  // flow through this bridge so the physical simulation and FINAL-06 ledger
  // cannot diverge.
  [[nodiscard]] const Simulation &physicalSimulation() const noexcept {
    return simulation_;
  }
  [[nodiscard]] const EconomyRuntime &economyRuntime() const noexcept {
    return economy_;
  }

  [[nodiscard]] std::string save() const;
  static SimulationEconomyBridge load(std::string_view data);
  [[nodiscard]] std::uint64_t authoritativeHash() const;

private:
  Simulation simulation_;
  EconomyRuntime economy_;
  EconomyView lastEconomy_{};
  std::int64_t lastFoodRevenueCents_{};
  std::int64_t lastEventRevenueCents_{};
  std::int64_t lastAmenityRevenueCents_{};
  std::int64_t cumulativeSellableRoomNights_{};
  std::int64_t cumulativeOccupiedRoomNights_{};

  void resetBaseline();
  void synchronizeCapacity();
  void reconcile(std::uint64_t sourceId, std::string_view memo);
  void recordDayBoundary(const SimulationView &view);
};

} // namespace hh::game
