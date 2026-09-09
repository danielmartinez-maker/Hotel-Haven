#include "hh/game/FoodService.h"
#include <stdexcept>
#include <string>
#include <vector>

using namespace hh::game;

static void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

static Recipe bakedEggs() {
  Recipe recipe;
  recipe.id = 1;
  recipe.name = "Baked eggs";
  recipe.ingredients = {{"eggs", 2}};
  recipe.requiredStation = FoodStationClass::Oven;
  recipe.prepSeconds = 2;
  recipe.cookSeconds = 2;
  recipe.plateSeconds = 1;
  recipe.qualityBase = 8600;
  recipe.priceCents = 1800;
  return recipe;
}

static FoodServiceSystem kitchen(bool oven) {
  FoodServiceSystem service;
  service.addRecipe(bakedEggs());
  service.setIngredientStock("eggs", 20);
  service.setStaffCapacity(2);
  if (oven)
    service.addStation({10, FoodStationClass::Oven, 1, true});
  return service;
}

static void ingredients_are_consumed_only_when_production_begins() {
  auto service = kitchen(true);
  const auto before = service.ingredientUnits("eggs");
  const auto order = service.createFoodOrder(
      101, {1, FoodOrderChannel::Breakfast, 1, 0});
  require(order != 0, "food order was not created");
  require(service.ingredientUnits("eggs") == before,
          "order creation consumed ingredients");
  require(service.beginProduction(order),
          "valid order did not begin production");
  require(service.ingredientUnits("eggs") == before - 2,
          "production did not consume exact ingredient quantity");
}

static void missing_required_station_blocks_with_explicit_reason() {
  auto service = kitchen(false);
  const auto order = service.createFoodOrder(
      102, {1, FoodOrderChannel::Breakfast, 1, 0});
  require(!service.beginProduction(order),
          "order without required station began production");
  const auto state = service.order(order);
  require(state.stage == FoodStage::Blocked,
          "missing station did not block order");
  require(state.blockReason == FoodBlockReason::MissingStation,
          "missing station did not expose MissingStation reason");
  require(service.ingredientUnits("eggs") == 20,
          "blocked order consumed ingredients");
}

static void production_is_deterministic_and_releases_capacity_when_ready() {
  auto service = kitchen(true);
  const auto first = service.createFoodOrder(
      201, {1, FoodOrderChannel::Breakfast, 1, 0});
  const auto second = service.createFoodOrder(
      202, {1, FoodOrderChannel::Breakfast, 1, 0});
  require(service.beginProduction(first),
          "first order did not claim station");
  require(!service.beginProduction(second),
          "second order exceeded station capacity");
  service.tickSeconds(5);
  require(service.order(first).stage == FoodStage::Ready,
          "order did not progress prep/cook/plate to ready");
  require(service.beginProduction(second),
          "capacity was not released when first order became ready");
  const auto restored = FoodServiceSystem::load(service.save());
  require(restored.save() == service.save(),
          "food-service save/load was not deterministic");
}

static void restaurant_visit_progresses_through_front_and_back_of_house() {
  auto service = kitchen(true);
  service.addVenue({50, FoodOrderChannel::Restaurant, 4, 0, 24 * 60,
                    1, 1, 1, 1, true});
  const auto order = service.createFoodOrder(
      301, {1, FoodOrderChannel::Restaurant, 1, 50});
  require(service.order(order).stage == FoodStage::AwaitingSeat,
          "restaurant order did not start by awaiting a seat");
  service.tickSeconds(12);
  const std::vector<FoodStage> expected{
      FoodStage::AwaitingSeat, FoodStage::Seated, FoodStage::Ordered,
      FoodStage::Prep, FoodStage::Cook, FoodStage::Plate, FoodStage::Ready,
      FoodStage::Served, FoodStage::Paid, FoodStage::Completed};
  require(service.stageHistory(order) == expected,
          "restaurant visit did not follow seat/order/produce/serve/pay/leave");
  require(service.order(order).stage == FoodStage::Completed,
          "restaurant visit did not complete");
  require(service.snapshot().revenueCents == 1800,
          "restaurant payment was not posted exactly once");
}

static void breakfast_window_blocks_service_outside_configured_hours() {
  auto service = kitchen(true);
  service.addVenue({60, FoodOrderChannel::Breakfast, 8, 6 * 60, 10 * 60,
                    1, 1, 1, 1, true});
  service.setElapsedSeconds(20 * 3600);
  const auto order = service.createFoodOrder(
      302, {1, FoodOrderChannel::Breakfast, 1, 60});
  service.tickSecond();
  const auto state = service.order(order);
  require(state.stage == FoodStage::Blocked,
          "breakfast order outside the service window was not blocked");
  require(state.blockReason == FoodBlockReason::OutsideServiceWindow,
          "breakfast window block reason was not explicit");
  require(service.ingredientUnits("eggs") == 20,
          "closed breakfast service consumed ingredients");
}

int main() {
  ingredients_are_consumed_only_when_production_begins();
  missing_required_station_blocks_with_explicit_reason();
  production_is_deterministic_and_releases_capacity_when_ready();
  restaurant_visit_progresses_through_front_and_back_of_house();
  breakfast_window_blocks_service_outside_configured_hours();
}
