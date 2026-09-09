#include "hh/game/FoodService.h"
#include <stdexcept>
#include <string>

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
  service.setIngredientStock("eggs", 10);
  service.setStaffCapacity(1);
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
  require(service.ingredientUnits("eggs") == 10,
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
          "second order exceeded station/staff capacity");
  service.tickSeconds(5);
  require(service.order(first).stage == FoodStage::Ready,
          "order did not progress prep/cook/plate to ready");
  require(service.beginProduction(second),
          "capacity was not released when first order became ready");
  const auto restored = FoodServiceSystem::load(service.save());
  require(restored.save() == service.save(),
          "food-service save/load was not deterministic");
}

int main() {
  ingredients_are_consumed_only_when_production_begins();
  missing_required_station_blocks_with_explicit_reason();
  production_is_deterministic_and_releases_capacity_when_ready();
}
