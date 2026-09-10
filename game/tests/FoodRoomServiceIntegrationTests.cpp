#include "hh/game/FoodService.h"
#include "hh/game/RoomService.h"
#include <stdexcept>

using namespace hh::game;

static void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

int main() {
  FoodServiceSystem food;
  Recipe recipe;
  recipe.id = 1;
  recipe.name = "Room-service breakfast";
  recipe.ingredients = {{"eggs", 2}};
  recipe.requiredStation = FoodStationClass::Oven;
  recipe.prepSeconds = 1;
  recipe.cookSeconds = 1;
  recipe.plateSeconds = 1;
  recipe.priceCents = 2400;
  food.addRecipe(recipe);
  food.setIngredientStock("eggs", 8);
  food.setStaffCapacity(1);
  food.addStation({10, FoodStationClass::Oven, 1, true});

  RoomServiceSystem logistics;
  RoomServiceOrder deliveryRequest;
  const auto delivery = logistics.placeRoomServiceOrder(7001, deliveryRequest);
  require(delivery != 0, "room-service logistics order was not created");

  const auto order = food.createFoodOrder(
      7001, {1, FoodOrderChannel::RoomService, 1, 0}, delivery);
  require(food.beginProduction(order), "room-service food did not begin production");
  food.tickSeconds(3);
  require(food.order(order).stage == FoodStage::Ready,
          "prepared room-service food did not become ready");

  auto transferPrepared = [&] {
    for (const auto &handoff : food.pendingRoomServiceHandoffs())
      if (logistics.markProductionReady(handoff.roomServiceOrderId))
        require(food.acknowledgeRoomServiceHandoff(handoff.foodOrderId),
                "successful logistics transfer was not acknowledged");
  };

  transferPrepared();
  transferPrepared();
  require(food.order(order).roomServiceTransferCount == 1,
          "prepared food transferred to logistics more than once");
  require(logistics.stage(delivery) == RoomServiceStage::RunnerPickup,
          "FINAL-04 logistics did not receive the prepared order");
  require(food.snapshot().revenueCents == 2400,
          "room-service revenue was duplicated or omitted");
}
