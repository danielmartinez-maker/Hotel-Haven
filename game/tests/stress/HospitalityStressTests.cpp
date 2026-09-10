#include "StressHarness.h"
#include "hh/game/Amenities.h"
#include "hh/game/Events.h"
#include "hh/game/FoodService.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace {
using namespace hh::game;

std::size_t operationBudget(hh::stress::Scale scale) {
  switch (scale) {
  case hh::stress::Scale::Pr:
    return 20'000;
  case hh::stress::Scale::Extended:
    return 200'000;
  case hh::stress::Scale::Exhaustive:
    return 1'000'000;
  }
  std::abort();
}

Recipe stressRecipe() {
  Recipe recipe;
  recipe.id = 1;
  recipe.name = "Stress bowl";
  recipe.ingredients = {{"grain", 1}};
  recipe.requiredStation = FoodStationClass::Prep;
  recipe.prepSeconds = 1;
  recipe.cookSeconds = 1;
  recipe.plateSeconds = 1;
  recipe.qualityBase = 8000;
  recipe.priceCents = 1500;
  return recipe;
}

FoodServiceSystem makeFood(std::size_t budget) {
  FoodServiceSystem food;
  food.addRecipe(stressRecipe());
  food.setIngredientStock("grain", static_cast<int>(std::min<std::size_t>(budget + 10'000, 2'000'000'000ULL)));
  food.setStaffCapacity(64);
  food.addStation({10, FoodStationClass::Prep, 64, true});
  food.addVenue({50, FoodOrderChannel::Restaurant, 128, 0, 24 * 60,
                 1, 1, 1, 1, true});
  return food;
}

void assertFood(const FoodServiceSnapshot &snapshot,
                hh::stress::RunContext &ctx, std::uint64_t checkpoint) {
  std::unordered_set<FoodOrderId> ids;
  for (const auto &stock : snapshot.inventory)
    if (stock.item.empty() || stock.units < 0)
      ctx.fail("negative or invalid food inventory", checkpoint);
  for (const auto &station : snapshot.stations)
    if (station.id == 0 || station.capacity <= 0 || station.active < 0 ||
        station.active > station.capacity)
      ctx.fail("food station capacity invariant violated", checkpoint);
  for (const auto &venue : snapshot.venues)
    if (venue.id == 0 || venue.seatCapacity <= 0 || venue.activeSeats < 0 ||
        venue.activeSeats > venue.seatCapacity)
      ctx.fail("food venue capacity invariant violated", checkpoint);
  for (const auto &order : snapshot.orders) {
    if (order.id == 0 || !ids.insert(order.id).second || order.guestId == 0 ||
        order.remainingStageSeconds < 0 || order.ageSeconds < 0 ||
        order.quality < 0 || order.quality > 10'000 || order.priceCents < 0 ||
        order.roomServiceTransferCount < 0 || order.roomServiceTransferCount > 1)
      ctx.fail("food order state invariant violated", checkpoint);
    const int stage = static_cast<int>(order.stage);
    if (stage < static_cast<int>(FoodStage::Queued) ||
        stage > static_cast<int>(FoodStage::Cancelled))
      ctx.fail("illegal food order stage", checkpoint);
  }
  if (snapshot.revenueCents < 0)
    ctx.fail("negative food-service revenue", checkpoint);
}

void assertAmenities(const AmenitiesSnapshot &snapshot,
                     hh::stress::RunContext &ctx,
                     std::uint64_t checkpoint) {
  std::unordered_set<AmenityReservationId> reservationIds;
  std::unordered_set<AmenityId> amenityIds;
  for (const auto &amenity : snapshot.amenities) {
    amenityIds.insert(amenity.id);
    if (amenity.id == 0 || amenity.capacity <= 0 || amenity.active < 0 ||
        amenity.active > amenity.capacity || amenity.cleanliness < 0 ||
        amenity.cleanliness > 10'000 || amenity.condition < 0 ||
        amenity.condition > 10'000 || amenity.priceCents < 0)
      ctx.fail("amenity capacity/condition invariant violated", checkpoint);
  }
  for (const auto &reservation : snapshot.reservations) {
    if (reservation.id == 0 || !reservationIds.insert(reservation.id).second ||
        reservation.guestId == 0 || !amenityIds.contains(reservation.amenityId) ||
        reservation.endSecond <= reservation.startSecond)
      ctx.fail("amenity reservation invariant violated", checkpoint);
    const int stage = static_cast<int>(reservation.stage);
    if (stage < static_cast<int>(AmenityStage::Reserved) ||
        stage > static_cast<int>(AmenityStage::Cancelled))
      ctx.fail("illegal amenity stage", checkpoint);
  }
  for (const auto &outcome : snapshot.outcomes)
    if (outcome.reservationId == 0 || outcome.guestId == 0 ||
        !amenityIds.contains(outcome.amenityId) || outcome.revenueCents < 0)
      ctx.fail("amenity outcome invariant violated", checkpoint);
  if (snapshot.revenueCents < 0)
    ctx.fail("negative amenity revenue", checkpoint);
}

void assertEvents(const EventsSnapshot &snapshot,
                  hh::stress::RunContext &ctx,
                  std::uint64_t checkpoint) {
  std::unordered_set<EventBookingId> bookingIds;
  for (const auto &booking : snapshot.bookings) {
    if (booking.id == 0 || !bookingIds.insert(booking.id).second ||
        booking.functionSpaceId == 0 || booking.endSecond <= booking.startSecond ||
        booking.attendees <= 0 || booking.mealServings < 0 ||
        booking.requiredStaffUnits < 0 || booking.contractCents < 0)
      ctx.fail("event booking invariant violated", checkpoint);
    const int phase = static_cast<int>(booking.phase);
    if (phase < static_cast<int>(EventPhase::Confirmed) ||
        phase > static_cast<int>(EventPhase::Cancelled))
      ctx.fail("illegal event phase", checkpoint);
  }
  for (const auto &workload : snapshot.workloads)
    if (!bookingIds.contains(workload.bookingId) || workload.staffUnits < 0 ||
        workload.mealServings < 0)
      ctx.fail("orphaned event workload", checkpoint);
  if (snapshot.revenueCents < 0)
    ctx.fail("negative event revenue", checkpoint);
}

void foodOrderBurst(const hh::stress::Config &config, std::size_t operations) {
  hh::stress::RunContext ctx{"hospitality", config};
  ctx.phase = "food_order_burst";
  auto food = makeFood(operations);
  std::vector<FoodOrderId> orders;
  orders.reserve(operations / 2 + 1);
  hh::stress::Rng rng{config.seed ^ 0xF00DF00DF00DF00DULL};

  for (std::size_t i = 0; i < operations; ++i) {
    if ((i & 1U) == 0U) {
      const auto id = food.createFoodOrder(
          static_cast<GuestId>(1000 + i),
          {1, FoodOrderChannel::RoomService, 1, 0},
          static_cast<RoomServiceOrderId>(500'000 + i));
      if (id == 0)
        ctx.fail("food order creation returned zero", i);
      orders.push_back(id);
      (void)food.beginProduction(id);
      ctx.trace.push("food:create=" + std::to_string(id));
    } else {
      food.tickSecond();
      if (!orders.empty()) {
        const auto id = orders[static_cast<std::size_t>(rng.bounded(orders.size()))];
        (void)food.beginProduction(id);
      }
      ctx.trace.push("food:tick");
    }
    if ((i & 255U) == 0U)
      assertFood(food.snapshot(), ctx, i);
  }
  food.tickSeconds(16);
  assertFood(food.snapshot(), ctx, operations);
  const auto saved = food.save();
  auto restored = FoodServiceSystem::load(saved);
  if (restored.save() != saved)
    ctx.fail("food-service save/load round-trip diverged", operations);
}

void inventoryStarvation(const hh::stress::Config &config) {
  hh::stress::RunContext ctx{"hospitality", config};
  ctx.phase = "inventory_starvation";
  FoodServiceSystem food;
  food.addRecipe(stressRecipe());
  food.setIngredientStock("grain", 0);
  food.setStaffCapacity(1);
  food.addStation({10, FoodStationClass::Prep, 1, true});
  const auto order = food.createFoodOrder(
      77, {1, FoodOrderChannel::RoomService, 1, 0}, 7001);
  if (food.beginProduction(order))
    ctx.fail("starved food order entered production", 0);
  const auto blocked = food.order(order);
  if (blocked.stage != FoodStage::Blocked ||
      blocked.blockReason != FoodBlockReason::MissingIngredient)
    ctx.fail("starved food order did not expose MissingIngredient", 1);
  food.setIngredientStock("grain", 10);
  if (!food.beginProduction(order))
    ctx.fail("food order did not recover after ingredient restoration", 2);
  food.tickSeconds(3);
  const auto ready = food.order(order);
  if (ready.stage != FoodStage::Ready)
    ctx.fail("recovered food order did not reach Ready", 5);
  const auto handoffs = food.pendingRoomServiceHandoffs();
  if (handoffs.size() != 1 || !food.acknowledgeRoomServiceHandoff(order))
    ctx.fail("room-service dependency handoff did not recover", 6);
  if (food.order(order).stage != FoodStage::Completed)
    ctx.fail("room-service food did not complete after handoff", 7);
  assertFood(food.snapshot(), ctx, 7);
}

void amenityCapacitySaturation(const hh::stress::Config &config,
                               std::size_t operations) {
  hh::stress::RunContext ctx{"hospitality", config};
  ctx.phase = "amenity_capacity_saturation";
  AmenitiesSystem amenities;
  AmenityConfig spa;
  spa.id = 10;
  spa.type = AmenityType::Spa;
  spa.openMinuteOfDay = 0;
  spa.closeMinuteOfDay = 24 * 60;
  spa.capacity = 4;
  spa.staffCapacity = 4;
  spa.priceCents = 5000;
  spa.defaultDurationSeconds = 4;
  amenities.addAmenity(spa);

  std::size_t accepted = 0;
  std::size_t rejected = 0;
  const std::size_t attempts = std::max<std::size_t>(100, operations / 4);
  for (std::size_t i = 0; i < attempts; ++i) {
    const std::int64_t slot = static_cast<std::int64_t>((i / 8) * 10 + 1);
    const auto result = amenities.reserveAmenity(
        static_cast<GuestId>(10'000 + i), {10, slot, 6});
    if (result.ok)
      ++accepted;
    else {
      ++rejected;
      if (result.blockReason != AmenityBlockReason::CapacityFull &&
          result.blockReason != AmenityBlockReason::StaffUnavailable)
        ctx.fail("amenity saturation rejected for unexpected reason", i);
    }
    if ((i & 255U) == 0U)
      assertAmenities(amenities.snapshot(), ctx, i);
  }
  if (accepted == 0 || rejected == 0)
    ctx.fail("amenity saturation did not exercise both acceptance and rejection", attempts);
  amenities.tickSeconds(static_cast<std::int64_t>((attempts / 8 + 2) * 10));
  assertAmenities(amenities.snapshot(), ctx, attempts);
  const auto saved = amenities.save();
  if (AmenitiesSystem::load(saved).save() != saved)
    ctx.fail("amenity save/load round-trip diverged", attempts);
}

void eventSetupTeardownChurn(const hh::stress::Config &config,
                             std::size_t operations) {
  hh::stress::RunContext ctx{"hospitality", config};
  ctx.phase = "event_setup_teardown_churn";
  EventsSystem events;
  events.addFunctionSpace({1, 200, true});
  events.setFoodServiceCapacity(500);
  events.setStaffCapacity(32);

  const std::size_t attempts = std::max<std::size_t>(100, operations / 8);
  std::size_t confirmed = 0;
  std::size_t conflicts = 0;
  for (std::size_t i = 0; i < attempts; ++i) {
    EventRequest request;
    request.functionSpaceId = 1;
    request.startSecond = static_cast<std::int64_t>((i / 2) * 20 + 10);
    request.durationSeconds = 5;
    request.attendees = 80;
    request.mealServings = 80;
    request.setupSeconds = 2;
    request.teardownSeconds = 2;
    request.requiredStaffUnits = 8;
    request.contractCents = 100'000;
    const auto quote = events.quoteEvent(request);
    if ((i & 1U) == 0U) {
      if (!quote.confirmable || events.confirmEvent(request) == 0)
        ctx.fail("non-overlapping event was not confirmable", i);
      ++confirmed;
    } else {
      if (quote.confirmable || quote.blockReason != EventBlockReason::TimeConflict)
        ctx.fail("overlapping event escaped TimeConflict", i);
      ++conflicts;
    }
    if ((i & 255U) == 0U)
      assertEvents(events.snapshot(), ctx, i);
  }
  if (confirmed == 0 || conflicts == 0)
    ctx.fail("event churn missed confirmation/conflict paths", attempts);
  events.tickSeconds(static_cast<std::int64_t>((attempts / 2 + 2) * 20));
  assertEvents(events.snapshot(), ctx, attempts);
  const auto saved = events.save();
  if (EventsSystem::load(saved).save() != saved)
    ctx.fail("event save/load round-trip diverged", attempts);
}

void roomServiceDependencyFailure(const hh::stress::Config &config) {
  hh::stress::RunContext ctx{"hospitality", config};
  ctx.phase = "room_service_dependency_failure";
  inventoryStarvation(config);
  ctx.trace.push("room-service:starvation-recovery");
}

void combinedHospitalityPeak(const hh::stress::Config &config,
                             std::size_t operations) {
  hh::stress::RunContext ctx{"hospitality", config};
  ctx.phase = "combined_hospitality_peak";
  auto food = makeFood(operations);
  AmenitiesSystem amenities;
  amenities.addAmenity({10, AmenityType::Gym, 0, 24 * 60, 16, 16, 10000,
                        10000, 1000, 5, true});
  EventsSystem events;
  events.addFunctionSpace({1, 500, true});
  events.setFoodServiceCapacity(1000);
  events.setStaffCapacity(64);
  hh::stress::Rng rng{config.seed ^ 0xC0B1A3D5ULL};
  std::int64_t nextEventStart = 10;

  for (std::size_t i = 0; i < operations; ++i) {
    switch (rng.bounded(6)) {
    case 0: {
      const auto id = food.createFoodOrder(
          static_cast<GuestId>(100'000 + i),
          {1, FoodOrderChannel::RoomService, 1, 0},
          static_cast<RoomServiceOrderId>(900'000 + i));
      (void)food.beginProduction(id);
      ctx.trace.push("food:create=" + std::to_string(id));
      break;
    }
    case 1:
      food.tickSecond();
      ctx.trace.push("food:tick");
      break;
    case 2: {
      const auto start = static_cast<std::int64_t>(i * 8 + 1);
      (void)amenities.reserveAmenity(static_cast<GuestId>(200'000 + i),
                                     {10, start, 5});
      ctx.trace.push("amenity:reserve");
      break;
    }
    case 3:
      amenities.tickSecond();
      ctx.trace.push("amenity:tick");
      break;
    case 4: {
      EventRequest request;
      request.functionSpaceId = 1;
      request.startSecond = nextEventStart;
      request.durationSeconds = 2;
      request.attendees = 100;
      request.mealServings = 100;
      request.setupSeconds = 1;
      request.teardownSeconds = 1;
      request.requiredStaffUnits = 10;
      request.contractCents = 50'000;
      if (events.confirmEvent(request) != 0)
        nextEventStart += 10;
      ctx.trace.push("event:confirm");
      break;
    }
    case 5:
      events.tickSecond();
      ctx.trace.push("event:tick");
      break;
    }

    if ((i & 255U) == 0U) {
      assertFood(food.snapshot(), ctx, i);
      assertAmenities(amenities.snapshot(), ctx, i);
      assertEvents(events.snapshot(), ctx, i);
    }
    if (i != 0 && i % 8192U == 0U) {
      const auto foodSave = food.save();
      const auto amenitySave = amenities.save();
      const auto eventSave = events.save();
      if (FoodServiceSystem::load(foodSave).save() != foodSave ||
          AmenitiesSystem::load(amenitySave).save() != amenitySave ||
          EventsSystem::load(eventSave).save() != eventSave)
        ctx.fail("hospitality peak save/load round-trip diverged", i);
      ctx.trace.push("combined:roundtrip");
    }
  }

  assertFood(food.snapshot(), ctx, operations);
  assertAmenities(amenities.snapshot(), ctx, operations);
  assertEvents(events.snapshot(), ctx, operations);
}

bool selected(const hh::stress::Config &config, std::string_view phase) {
  return config.scenario.empty() || config.scenario == phase;
}

void validateScenario(const hh::stress::Config &config) {
  if (config.scenario.empty())
    return;
  for (std::string_view valid : {"food_order_burst", "inventory_starvation",
                                 "amenity_capacity_saturation",
                                 "event_setup_teardown_churn",
                                 "room_service_dependency_failure",
                                 "combined_hospitality_peak"})
    if (config.scenario == valid)
      return;
  throw std::invalid_argument(
      "HH_STRESS_SCENARIO must be food_order_burst, inventory_starvation, "
      "amenity_capacity_saturation, event_setup_teardown_churn, "
      "room_service_dependency_failure, or combined_hospitality_peak");
}
} // namespace

int main() {
  try {
    const auto config =
        hh::stress::configFromEnvironment(0x5EED05055EED0505ULL);
    validateScenario(config);
    const auto operations = operationBudget(config.scale);
    if (selected(config, "food_order_burst"))
      foodOrderBurst(config, operations);
    if (selected(config, "inventory_starvation"))
      inventoryStarvation(config);
    if (selected(config, "amenity_capacity_saturation"))
      amenityCapacitySaturation(config, operations);
    if (selected(config, "event_setup_teardown_churn"))
      eventSetupTeardownChurn(config, operations);
    if (selected(config, "room_service_dependency_failure"))
      roomServiceDependencyFailure(config);
    if (selected(config, "combined_hospitality_peak"))
      combinedHospitalityPeak(config, operations);
    std::cout << "StressHospitality PASS operations=" << operations << '\n';
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
