#pragma once

#include "hh/game/ServiceTypes.h"
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace hh::game {

using FoodOrderId = ServiceId;
using RecipeId = ServiceId;
using FoodStationId = ServiceId;
using VenueId = ServiceId;

enum class FoodStationClass : std::uint8_t {
  Prep,
  Range,
  Oven,
  Bar,
  Plating,
  ServicePass
};
enum class FoodOrderChannel : std::uint8_t {
  Restaurant,
  Bar,
  Breakfast,
  RoomService,
  Banquet
};
enum class FoodStage : std::uint8_t {
  Queued,
  Blocked,
  Prep,
  Cook,
  Plate,
  Ready,
  Served,
  Paid,
  Completed,
  Cancelled
};
enum class FoodBlockReason : std::uint8_t {
  None,
  MissingRecipe,
  InvalidOrder,
  MissingIngredient,
  MissingStation,
  StationCapacity,
  NoStaffCapacity,
  OutsideServiceWindow,
  VenueCapacity
};

struct IngredientRequirement {
  std::string item;
  int units{};
};
struct IngredientStockView {
  std::string item;
  int units{};
};
struct Recipe {
  RecipeId id{};
  std::string name;
  std::vector<IngredientRequirement> ingredients;
  FoodStationClass requiredStation{FoodStationClass::Prep};
  int prepSeconds{1};
  int cookSeconds{1};
  int plateSeconds{1};
  int qualityBase{8000};
  std::int64_t priceCents{};
};
struct FoodStationConfig {
  FoodStationId id{};
  FoodStationClass stationClass{FoodStationClass::Prep};
  int capacity{1};
  bool enabled{true};
};
struct MenuOrder {
  RecipeId recipeId{};
  FoodOrderChannel channel{FoodOrderChannel::Restaurant};
  int quantity{1};
  VenueId venueId{};
};
struct FoodOrderView {
  FoodOrderId id{};
  GuestId guestId{};
  RecipeId recipeId{};
  FoodOrderChannel channel{FoodOrderChannel::Restaurant};
  FoodStage stage{FoodStage::Queued};
  FoodBlockReason blockReason{FoodBlockReason::None};
  int remainingStageSeconds{};
  int ageSeconds{};
  int quality{};
  std::int64_t priceCents{};
  RoomServiceOrderId roomServiceOrderId{};
  int roomServiceTransferCount{};
};
struct FoodStationView {
  FoodStationId id{};
  FoodStationClass stationClass{FoodStationClass::Prep};
  int capacity{};
  int active{};
  bool enabled{};
};
struct PreparedRoomServiceHandoff {
  FoodOrderId foodOrderId{};
  RoomServiceOrderId roomServiceOrderId{};
};
struct FoodServiceSnapshot {
  std::int64_t elapsedSeconds{};
  std::vector<IngredientStockView> inventory;
  std::vector<FoodStationView> stations;
  std::vector<FoodOrderView> orders;
  std::int64_t revenueCents{};
};

class FoodServiceSystem {
public:
  FoodServiceSystem();
  ~FoodServiceSystem();
  FoodServiceSystem(FoodServiceSystem &&) noexcept;
  FoodServiceSystem &operator=(FoodServiceSystem &&) noexcept;
  FoodServiceSystem(const FoodServiceSystem &);
  FoodServiceSystem &operator=(const FoodServiceSystem &);

  void addRecipe(const Recipe &recipe);
  void setIngredientStock(std::string item, int units);
  [[nodiscard]] int ingredientUnits(std::string_view item) const;
  void addStation(const FoodStationConfig &station);
  void setStaffCapacity(int capacity);

  [[nodiscard]] FoodOrderId createFoodOrder(
      GuestId guest, const MenuOrder &order,
      RoomServiceOrderId roomServiceOrderId = 0);
  [[nodiscard]] bool beginProduction(FoodOrderId id);
  void tickSecond();
  void tickSeconds(std::int64_t seconds);

  [[nodiscard]] FoodOrderView order(FoodOrderId id) const;
  [[nodiscard]] std::vector<FoodStage> stageHistory(FoodOrderId id) const;
  [[nodiscard]] FoodServiceSnapshot snapshot() const;
  [[nodiscard]] std::vector<PreparedRoomServiceHandoff>
  pendingRoomServiceHandoffs() const;
  [[nodiscard]] bool acknowledgeRoomServiceHandoff(FoodOrderId id);

  [[nodiscard]] std::string save() const;
  static FoodServiceSystem load(std::string_view data);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace hh::game
