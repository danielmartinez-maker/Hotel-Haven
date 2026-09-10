#include "hh/game/Simulation.h"
#include <stdexcept>
#include <string>

using namespace hh::game;

static void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

static FoodOrderView foodOrder(const FoodServiceSnapshot &snapshot,
                               FoodOrderId id) {
  for (const auto &order : snapshot.orders)
    if (order.id == id)
      return order;
  throw std::runtime_error("food order missing from simulation snapshot");
}

int main() {
  auto sim = Simulation::tutorial(505);
  const auto startingRevenue = sim.view().economy.revenueCents;

  const auto initialFood = sim.foodServiceSnapshot();
  require(initialFood.venues.size() == 3,
          "tutorial did not initialize restaurant/bar/breakfast venues");
  require(sim.amenitiesSnapshot().amenities.size() == 3,
          "tutorial did not initialize gym/spa/pool amenities");

  const auto restaurant = sim.createFoodOrder(
      9001, {2, FoodOrderChannel::Restaurant, 1, 102});
  require(restaurant != 0, "simulation rejected a restaurant order");
  sim.step(500);
  auto food = sim.foodServiceSnapshot();
  require(foodOrder(food, restaurant).stage == FoodStage::Completed,
          "restaurant order did not traverse the full service lifecycle");
  require(food.revenueCents == 3200,
          "restaurant revenue was not posted exactly once");
  require(sim.view().economy.revenueCents == startingRevenue + 3200,
          "restaurant revenue did not reach the authoritative economy ledger");

  const auto roomService = sim.createFoodOrder(
      9002, {1, FoodOrderChannel::RoomService, 1, 0});
  require(roomService != 0, "simulation rejected a room-service food order");
  sim.step(200);
  food = sim.foodServiceSnapshot();
  const auto prepared = foodOrder(food, roomService);
  require(prepared.stage == FoodStage::Completed,
          "prepared room-service order did not hand off to FINAL-04");
  require(prepared.roomServiceOrderId != 0 &&
              prepared.roomServiceTransferCount == 1,
          "room-service production did not transfer exactly once");
  require(food.revenueCents == 5000,
          "room-service revenue was omitted or duplicated");

  EventRequest event;
  event.functionSpaceId = 301;
  event.startSecond = sim.view().elapsedSeconds + 60;
  event.durationSeconds = 120;
  event.attendees = 40;
  event.mealServings = 40;
  event.setupSeconds = 30;
  event.teardownSeconds = 30;
  event.requiredStaffUnits = 5;
  event.contractCents = 250000;
  const auto quote = sim.quoteEvent(event);
  require(quote.confirmable && quote.functionSpaceId == 301,
          "simulation did not expose a feasible event quote");
  const auto booking = sim.confirmEvent(event);
  require(booking != 0, "simulation did not confirm a feasible event");
  sim.step(250);
  const auto events = sim.eventsSnapshot();
  require(events.bookings.size() == 1 &&
              events.bookings.front().phase == EventPhase::Completed,
          "event did not traverse setup/service/teardown to completion");
  require(events.revenueCents == 250000,
          "event contract revenue was not posted exactly once");

  AmenityRequest spa;
  spa.amenityId = 202;
  spa.startSecond = sim.view().elapsedSeconds + 60;
  spa.durationSeconds = 120;
  const auto reservation = sim.reserveAmenity(9003, spa);
  require(reservation.ok && reservation.reservationId != 0,
          "simulation rejected a feasible spa reservation");
  sim.step(200);
  const auto amenities = sim.amenitiesSnapshot();
  require(amenities.outcomes.size() == 1 &&
              amenities.outcomes.front().guestId == 9003,
          "amenity completion did not publish a guest-attributed outcome");
  require(amenities.revenueCents == 15000,
          "spa revenue was not posted exactly once");

  require(sim.view().economy.revenueCents == startingRevenue + 270000,
          "FINAL-05 revenue was not synchronized exactly once to the hotel ledger");

  const auto encoded = sim.save();
  require(encoded.rfind("HHGS 9 ", 0) == 0,
          "FINAL-05 state did not advance the game save schema to HHGS 9");
  auto restored = Simulation::load(encoded);
  require(restored.save() == encoded,
          "FINAL-05 state did not round-trip through Simulation save/load");
  sim.step(120);
  restored.step(120);
  require(restored.save() == sim.save(),
          "restored FINAL-05 simulation diverged after deterministic continuation");
}
