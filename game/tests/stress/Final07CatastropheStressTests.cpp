#include "StressHarness.h"
#include "hh/game/Amenities.h"
#include "hh/game/EconomyRuntime.h"
#include "hh/game/Events.h"
#include "hh/game/FoodService.h"
#include "hh/game/ServiceLogistics.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>

namespace {
using namespace hh::game;

struct ScaleBudget {
  std::size_t operations;
  int economyDays;
};

ScaleBudget budget(hh::stress::Scale scale) {
  switch (scale) {
  case hh::stress::Scale::Pr:
    return {20'000, 30};
  case hh::stress::Scale::Extended:
    return {100'000, 365};
  case hh::stress::Scale::Exhaustive:
    return {500'000, 2'000};
  }
  std::abort();
}

Recipe crisisRecipe() {
  Recipe recipe;
  recipe.id = 1;
  recipe.name = "Crisis meal";
  recipe.ingredients = {{"ingredient", 1}};
  recipe.requiredStation = FoodStationClass::Prep;
  recipe.prepSeconds = 1;
  recipe.cookSeconds = 1;
  recipe.plateSeconds = 1;
  recipe.qualityBase = 7500;
  recipe.priceCents = 1800;
  return recipe;
}

void assertService(ServiceLogisticsRuntime &service,
                   hh::stress::RunContext &ctx,
                   std::uint64_t checkpoint) {
  const auto logistics = service.logisticsSnapshot();
  std::unordered_set<StorageNodeId> storage;
  for (const auto &node : logistics.storage) {
    storage.insert(node.id);
    if (node.id == 0 || node.capacityUnits <= 0 || node.usedUnits < 0 ||
        node.reservedUnits < 0 || node.usedUnits > node.capacityUnits ||
        node.reservedUnits > node.capacityUnits)
      ctx.fail("service: storage invariant violated", checkpoint);
  }
  for (const auto &stack : logistics.inventory)
    if (!storage.contains(stack.storage) || stack.quantity < 0 ||
        stack.reservedQuantity < 0 || stack.reservedQuantity > stack.quantity)
      ctx.fail("service: inventory invariant violated", checkpoint);
  if (logistics.wasteAtSources < 0 || logistics.wasteInBackOfHouse < 0 ||
      logistics.wasteOverflowUnits < 0)
    ctx.fail("service: waste invariant violated", checkpoint);
  for (const auto &job : service.housekeeping().snapshot().jobs)
    if (job.id == 0 || job.roomId == 0 || job.remainingSeconds < 0)
      ctx.fail("service: housekeeping job invariant violated", checkpoint);
  for (const auto &batch : service.laundry().snapshot().batches)
    if (batch.id == 0 || batch.quantity <= 0 || batch.remainingSeconds < 0)
      ctx.fail("service: laundry batch invariant violated", checkpoint);
  for (const auto &order : service.engineering().snapshot().workOrders)
    if (order.id == 0 || order.assetId == 0 || order.remainingSeconds < 0)
      ctx.fail("service: engineering work-order invariant violated", checkpoint);
}

void assertHospitality(const FoodServiceSystem &food,
                       const AmenitiesSystem &amenities,
                       const EventsSystem &events,
                       hh::stress::RunContext &ctx,
                       std::uint64_t checkpoint) {
  const auto foodSnapshot = food.snapshot();
  for (const auto &stock : foodSnapshot.inventory)
    if (stock.units < 0)
      ctx.fail("hospitality: negative food inventory", checkpoint);
  for (const auto &station : foodSnapshot.stations)
    if (station.active < 0 || station.active > station.capacity)
      ctx.fail("hospitality: food station capacity exceeded", checkpoint);
  for (const auto &venue : foodSnapshot.venues)
    if (venue.activeSeats < 0 || venue.activeSeats > venue.seatCapacity)
      ctx.fail("hospitality: venue capacity exceeded", checkpoint);
  for (const auto &order : foodSnapshot.orders)
    if (order.id == 0 || order.guestId == 0 || order.remainingStageSeconds < 0 ||
        order.roomServiceTransferCount < 0 || order.roomServiceTransferCount > 1)
      ctx.fail("hospitality: food order invariant violated", checkpoint);

  const auto amenitySnapshot = amenities.snapshot();
  for (const auto &amenity : amenitySnapshot.amenities)
    if (amenity.active < 0 || amenity.active > amenity.capacity ||
        amenity.cleanliness < 0 || amenity.cleanliness > 10'000 ||
        amenity.condition < 0 || amenity.condition > 10'000)
      ctx.fail("hospitality: amenity invariant violated", checkpoint);

  const auto eventSnapshot = events.snapshot();
  std::unordered_set<EventBookingId> eventIds;
  for (const auto &booking : eventSnapshot.bookings)
    if (booking.id == 0 || !eventIds.insert(booking.id).second ||
        booking.endSecond <= booking.startSecond || booking.attendees <= 0 ||
        booking.mealServings < 0 || booking.requiredStaffUnits < 0)
      ctx.fail("hospitality: event booking invariant violated", checkpoint);
}

void assertEconomy(const EconomyRuntime &economy,
                   hh::stress::RunContext &ctx,
                   std::uint64_t checkpoint) {
  const auto market = economy.marketSnapshot();
  if (market.generatedRequests !=
      market.playerWins + market.competitorWins + market.unallocatedRequests)
    ctx.fail("economy: market allocation conservation violated", checkpoint,
             std::to_string(economy.authoritativeHash()));
  const auto revenue = economy.revenueManagementSnapshot();
  if (revenue.inventory.minimumAvailableUnits < 0)
    ctx.fail("economy: inventory exceeded explicit overbooking allowance", checkpoint,
             std::to_string(economy.authoritativeHash()));
  const auto finance = economy.financialSnapshot();
  if (!finance.economics.reconciled || finance.financing.outstandingPrincipalCents < 0 ||
      finance.financing.missedObligationCents < 0)
    ctx.fail("economy: financial reconciliation invariant violated", checkpoint,
             std::to_string(economy.authoritativeHash()));
  if (economy.estimatedStateBytes() > 64ULL * 1024ULL * 1024ULL)
    ctx.fail("economy: state exceeded supported save envelope", checkpoint);
}

void configureEconomy(EconomyRuntime &economy, bool financialCrisis) {
  economy.setPhysicalRoomCapacity("standard", 120);
  economy.setPlayerHotelOffer({1, 15'000, 76, 4, 78, 80, 75, true});
  economy.setCompetitors({{2, "Rival", 14'000, 75, 4, 80, 80, 75},
                          {3, "Premium", 22'000, 88, 5, 90, 90, 88}});
  if (!economy.setPricingRule({1, 0, 10'000, 0x7f, "standard", 0, 10000,
                               15'000, 1, 8'000, 35'000}).ok)
    throw std::runtime_error("catastrophe pricing rule rejected");
  if (!economy.setOverbookingPolicy({"standard", 4, 30'000}).ok)
    throw std::runtime_error("catastrophe overbooking policy rejected");
  if (financialCrisis) {
    LoanOffer loan;
    loan.id = 700;
    loan.principalCents = 1'500'000;
    loan.annualInterestBasisPoints = 2'400;
    loan.termMonths = 6;
    loan.paymentFrequencyDays = 30;
    loan.originationFeeCents = 100'000;
    if (!economy.acceptLoan(loan).ok)
      throw std::runtime_error("catastrophe financing fixture rejected");
  }
}

void roundTripAndTail(ServiceLogisticsRuntime &service,
                      FoodServiceSystem &food,
                      AmenitiesSystem &amenities,
                      EventsSystem &events,
                      EconomyRuntime &economy,
                      hh::stress::RunContext &ctx,
                      std::uint64_t checkpoint) {
  const auto serviceSave = service.save();
  const auto foodSave = food.save();
  const auto amenitiesSave = amenities.save();
  const auto eventsSave = events.save();
  const auto economySave = economy.save();

  auto serviceRestored = ServiceLogisticsRuntime::load(serviceSave);
  auto foodRestored = FoodServiceSystem::load(foodSave);
  auto amenitiesRestored = AmenitiesSystem::load(amenitiesSave);
  auto eventsRestored = EventsSystem::load(eventsSave);
  auto economyRestored = EconomyRuntime::load(economySave);

  if (serviceRestored.save() != serviceSave || foodRestored.save() != foodSave ||
      amenitiesRestored.save() != amenitiesSave ||
      eventsRestored.save() != eventsSave || economyRestored.save() != economySave)
    ctx.fail("save_in_crisis: authoritative round-trip diverged", checkpoint);

  service.tickSeconds(30);
  serviceRestored.tickSeconds(30);
  food.tickSeconds(8);
  foodRestored.tickSeconds(8);
  amenities.tickSeconds(8);
  amenitiesRestored.tickSeconds(8);
  events.tickSeconds(8);
  eventsRestored.tickSeconds(8);
  economy.runDays(5);
  economyRestored.runDays(5);

  if (serviceRestored.save() != service.save() ||
      foodRestored.save() != food.save() ||
      amenitiesRestored.save() != amenities.save() ||
      eventsRestored.save() != events.save() ||
      economyRestored.authoritativeHash() != economy.authoritativeHash())
    ctx.fail("save_in_crisis: deterministic continuation diverged", checkpoint,
             std::to_string(economy.authoritativeHash()) + "/" +
                 std::to_string(economyRestored.authoritativeHash()));
}

void runScenario(std::string_view scenario,
                 const hh::stress::Config &config,
                 const ScaleBudget &limits) {
  hh::stress::RunContext ctx{"final07-catastrophe", config};
  ctx.phase = std::string(scenario);
  const bool inventoryCrunch = scenario == "inventory_and_service_crunch";
  const bool financialCrisis =
      scenario == "financial_crisis_with_service_pressure" ||
      scenario == "save_in_crisis";

  ServiceLogisticsRuntime service(config.seed ^ 0x0404ULL, {2, 2, 2});
  const int serviceStock = inventoryCrunch ? 2 : 100;
  if (!service.setScenarioInventory(serviceStock, serviceStock * 2,
                                    serviceStock, serviceStock, serviceStock))
    ctx.fail("service: scenario inventory initialization failed", 0);
  for (int room = 0; room < 200; ++room)
    service.registerRoom(static_cast<RoomId>(10'000 + room), ServiceRoomStatus::Ready);
  for (int asset = 0; asset < 50; ++asset)
    service.registerAsset(static_cast<AssetId>(20'000 + asset), 3500 + asset * 50);

  FoodServiceSystem food;
  food.addRecipe(crisisRecipe());
  food.setIngredientStock("ingredient", inventoryCrunch ? 0 : 25'000);
  food.setStaffCapacity(inventoryCrunch ? 2 : 16);
  food.addStation({10, FoodStationClass::Prep, inventoryCrunch ? 1 : 16, true});

  AmenitiesSystem amenities;
  amenities.addAmenity({10, AmenityType::Spa, 0, 24 * 60, 4, 4, 10000,
                        10000, 5000, 10, true});

  EventsSystem events;
  events.addFunctionSpace({1, 250, true});
  events.setFoodServiceCapacity(inventoryCrunch ? 100 : 1000);
  events.setStaffCapacity(inventoryCrunch ? 5 : 30);

  EconomyRuntime economy(config.seed ^ 0x0606ULL,
                         financialCrisis ? 0 : 100'000'000);
  configureEconomy(economy, financialCrisis);

  hh::stress::Rng rng{config.seed ^ 0x0707070707070707ULL};
  std::int64_t nextEventStart = 10;
  const std::size_t operations = limits.operations;
  for (std::size_t op = 0; op < operations; ++op) {
    const auto action = rng.bounded(10);
    const auto room = static_cast<RoomId>(10'000 + rng.bounded(200));
    const auto asset = static_cast<AssetId>(20'000 + rng.bounded(50));
    switch (action) {
    case 0:
      (void)service.requestRoomTurn(room);
      ctx.trace.push("service:turn=" + std::to_string(room));
      break;
    case 1:
      (void)service.workRoomTurnSecond(room);
      service.tickSecond();
      ctx.trace.push("service:work");
      break;
    case 2:
      (void)service.createWorkOrder(asset, WorkOrderType::Corrective);
      ctx.trace.push("service:repair=" + std::to_string(asset));
      break;
    case 3: {
      const auto id = food.createFoodOrder(
          static_cast<GuestId>(100'000 + op),
          {1, FoodOrderChannel::RoomService, 1, 0},
          static_cast<RoomServiceOrderId>(500'000 + op));
      (void)food.beginProduction(id);
      ctx.trace.push("hospitality:food=" + std::to_string(id));
      break;
    }
    case 4:
      food.tickSecond();
      ctx.trace.push("hospitality:food-tick");
      break;
    case 5:
      (void)amenities.reserveAmenity(
          static_cast<GuestId>(200'000 + op),
          {10, static_cast<std::int64_t>(op * 12 + 1), 10});
      ctx.trace.push("hospitality:amenity");
      break;
    case 6:
      amenities.tickSecond();
      ctx.trace.push("hospitality:amenity-tick");
      break;
    case 7: {
      EventRequest request;
      request.functionSpaceId = 1;
      request.startSecond = nextEventStart;
      request.durationSeconds = 4;
      request.attendees = inventoryCrunch ? 80 : 120;
      request.mealServings = request.attendees;
      request.setupSeconds = 2;
      request.teardownSeconds = 2;
      request.requiredStaffUnits = inventoryCrunch ? 4 : 10;
      request.contractCents = 80'000;
      if (events.confirmEvent(request) != 0)
        nextEventStart += 12;
      ctx.trace.push("hospitality:event");
      break;
    }
    case 8:
      events.tickSecond();
      ctx.trace.push("hospitality:event-tick");
      break;
    case 9:
      service.logistics().produceWaste(1 + static_cast<int>(rng.bounded(3)));
      if ((rng.next() & 31ULL) == 0)
        service.logistics().requestWastePickup();
      ctx.trace.push("service:waste");
      break;
    }

    if (inventoryCrunch && op == operations / 2) {
      food.setIngredientStock("ingredient", 25'000);
      ctx.trace.push("hospitality:restock");
    }

    if ((op & 511U) == 0U) {
      assertService(service, ctx, op);
      assertHospitality(food, amenities, events, ctx, op);
    }
  }

  economy.runDays(limits.economyDays);
  assertService(service, ctx, operations);
  assertHospitality(food, amenities, events, ctx, operations);
  assertEconomy(economy, ctx, static_cast<std::uint64_t>(limits.economyDays));

  if (scenario == "overbooking_and_cancellation_reversal") {
    const auto inventory = economy.revenueManagementSnapshot().inventory;
    if (inventory.minimumAvailableUnits < 0)
      ctx.fail("economy: overbooking reversal exceeded configured allowance",
               limits.economyDays);
  }

  if (financialCrisis) {
    const auto financing = economy.financialSnapshot().financing;
    if (financing.outstandingPrincipalCents < 0 ||
        financing.missedObligationCents < 0)
      ctx.fail("economy: crisis financing state invalid", limits.economyDays);
  }

  if (scenario == "save_in_crisis")
    roundTripAndTail(service, food, amenities, events, economy, ctx,
                     operations);
}

void validateScenario(const hh::stress::Config &config) {
  if (config.scenario.empty())
    return;
  for (std::string_view valid : {
           "hospitality_demand_shock", "inventory_and_service_crunch",
           "overbooking_and_cancellation_reversal",
           "financial_crisis_with_service_pressure", "save_in_crisis"})
    if (config.scenario == valid)
      return;
  throw std::invalid_argument(
      "unknown FINAL-07 catastrophe scenario; expected hospitality_demand_shock, "
      "inventory_and_service_crunch, overbooking_and_cancellation_reversal, "
      "financial_crisis_with_service_pressure, or save_in_crisis");
}
} // namespace

int main() {
  try {
    const auto config =
        hh::stress::configFromEnvironment(0x5EED07075EED0707ULL);
    validateScenario(config);
    const auto limits = budget(config.scale);
    if (config.scenario.empty()) {
      for (std::string_view scenario : {
               "hospitality_demand_shock", "inventory_and_service_crunch",
               "overbooking_and_cancellation_reversal",
               "financial_crisis_with_service_pressure", "save_in_crisis"})
        runScenario(scenario, config, limits);
    } else {
      runScenario(config.scenario, config, limits);
    }
    std::cout << "StressFinal07Catastrophe PASS operations_per_scenario="
              << limits.operations << " economy_days=" << limits.economyDays
              << '\n';
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
