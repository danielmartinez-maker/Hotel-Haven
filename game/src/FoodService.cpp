#include "hh/game/FoodService.h"

namespace hh::game {

void FoodServiceSystem::addRecipe(const Recipe &) {}
void FoodServiceSystem::setIngredientStock(std::string, int) {}
int FoodServiceSystem::ingredientUnits(std::string_view) const { return 0; }
void FoodServiceSystem::addStation(const FoodStationConfig &) {}
void FoodServiceSystem::setStaffCapacity(int) {}
FoodOrderId FoodServiceSystem::createFoodOrder(GuestId, const MenuOrder &,
                                               RoomServiceOrderId) {
  return 0;
}
bool FoodServiceSystem::beginProduction(FoodOrderId) { return false; }
void FoodServiceSystem::tickSecond() {}
void FoodServiceSystem::tickSeconds(std::int64_t) {}
FoodOrderView FoodServiceSystem::order(FoodOrderId) const { return {}; }
std::vector<FoodStage> FoodServiceSystem::stageHistory(FoodOrderId) const {
  return {};
}
FoodServiceSnapshot FoodServiceSystem::snapshot() const { return {}; }
std::vector<PreparedRoomServiceHandoff>
FoodServiceSystem::pendingRoomServiceHandoffs() const {
  return {};
}
bool FoodServiceSystem::acknowledgeRoomServiceHandoff(FoodOrderId) {
  return false;
}
std::string FoodServiceSystem::save() const { return {}; }
FoodServiceSystem FoodServiceSystem::load(std::string_view) { return {}; }

} // namespace hh::game
