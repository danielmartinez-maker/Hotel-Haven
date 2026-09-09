#include "hh/game/FoodService.h"

#include <algorithm>
#include <iomanip>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>

namespace hh::game {
namespace {
template <class E> int ei(E value) { return static_cast<int>(value); }

bool validStationClass(int value) {
  return value >= ei(FoodStationClass::Prep) &&
         value <= ei(FoodStationClass::ServicePass);
}
bool validChannel(int value) {
  return value >= ei(FoodOrderChannel::Restaurant) &&
         value <= ei(FoodOrderChannel::Banquet);
}
bool validStage(int value) {
  return value >= ei(FoodStage::Queued) && value <= ei(FoodStage::Cancelled);
}
bool validBlock(int value) {
  return value >= ei(FoodBlockReason::None) &&
         value <= ei(FoodBlockReason::VenueCapacity);
}
} // namespace

struct FoodServiceSystem::Impl {
  struct Station : FoodStationView {};
  struct ActiveOrder : FoodOrderView {
    int quantity{1};
    FoodStationId stationId{};
    bool resourcesClaimed{};
    std::vector<FoodStage> history;
  };

  ServiceId nextId{1};
  std::int64_t elapsedSeconds{};
  std::map<RecipeId, Recipe> recipes;
  std::map<std::string, int, std::less<>> inventory;
  std::vector<Station> stations;
  std::vector<ActiveOrder> orders;
  int staffCapacity{};
  int activeStaff{};
  std::int64_t revenueCents{};

  ActiveOrder *findOrder(FoodOrderId id) {
    for (auto &value : orders)
      if (value.id == id)
        return &value;
    return nullptr;
  }
  const ActiveOrder *findOrder(FoodOrderId id) const {
    for (const auto &value : orders)
      if (value.id == id)
        return &value;
    return nullptr;
  }
  Station *findStation(FoodStationId id) {
    for (auto &station : stations)
      if (station.id == id)
        return &station;
    return nullptr;
  }
  void enter(ActiveOrder &order, FoodStage stage, int seconds = 0,
             FoodBlockReason block = FoodBlockReason::None) {
    order.stage = stage;
    order.blockReason = block;
    order.remainingStageSeconds = std::max(0, seconds);
    if (order.history.empty() || order.history.back() != stage)
      order.history.push_back(stage);
  }
  void releaseProduction(ActiveOrder &order) {
    if (!order.resourcesClaimed)
      return;
    if (auto *station = findStation(order.stationId))
      station->active = std::max(0, station->active - 1);
    activeStaff = std::max(0, activeStaff - 1);
    order.resourcesClaimed = false;
    order.stationId = 0;
  }
  void advanceStage(ActiveOrder &order, const Recipe &recipe) {
    for (;;) {
      if (order.stage == FoodStage::Prep) {
        if (recipe.cookSeconds > 0) {
          enter(order, FoodStage::Cook, recipe.cookSeconds);
          return;
        }
        order.stage = FoodStage::Cook;
      }
      if (order.stage == FoodStage::Cook) {
        if (recipe.plateSeconds > 0) {
          enter(order, FoodStage::Plate, recipe.plateSeconds);
          return;
        }
        order.stage = FoodStage::Plate;
      }
      if (order.stage == FoodStage::Plate) {
        enter(order, FoodStage::Ready);
        releaseProduction(order);
        return;
      }
      return;
    }
  }
};

FoodServiceSystem::FoodServiceSystem() : impl_(std::make_unique<Impl>()) {}
FoodServiceSystem::~FoodServiceSystem() = default;
FoodServiceSystem::FoodServiceSystem(FoodServiceSystem &&) noexcept = default;
FoodServiceSystem &FoodServiceSystem::operator=(FoodServiceSystem &&) noexcept =
    default;
FoodServiceSystem::FoodServiceSystem(const FoodServiceSystem &other)
    : impl_(std::make_unique<Impl>(*other.impl_)) {}
FoodServiceSystem &
FoodServiceSystem::operator=(const FoodServiceSystem &other) {
  if (this != &other)
    impl_ = std::make_unique<Impl>(*other.impl_);
  return *this;
}

void FoodServiceSystem::addRecipe(const Recipe &recipe) {
  if (recipe.id == 0 || recipe.name.empty() || recipe.prepSeconds < 0 ||
      recipe.cookSeconds < 0 || recipe.plateSeconds < 0 ||
      recipe.qualityBase < 0 || recipe.qualityBase > 10000 ||
      recipe.priceCents < 0)
    throw std::invalid_argument("invalid food recipe");
  for (const auto &ingredient : recipe.ingredients)
    if (ingredient.item.empty() || ingredient.units <= 0)
      throw std::invalid_argument("invalid food ingredient requirement");
  impl_->recipes[recipe.id] = recipe;
}

void FoodServiceSystem::setIngredientStock(std::string item, int units) {
  if (item.empty() || units < 0)
    throw std::invalid_argument("invalid ingredient stock");
  impl_->inventory[std::move(item)] = units;
}

int FoodServiceSystem::ingredientUnits(std::string_view item) const {
  const auto found = impl_->inventory.find(item);
  return found == impl_->inventory.end() ? 0 : found->second;
}

void FoodServiceSystem::addStation(const FoodStationConfig &station) {
  if (station.id == 0 || station.capacity <= 0)
    throw std::invalid_argument("invalid food station");
  if (std::any_of(impl_->stations.begin(), impl_->stations.end(),
                  [&](const auto &value) { return value.id == station.id; }))
    throw std::invalid_argument("duplicate food station");
  Impl::Station value;
  value.id = station.id;
  value.stationClass = station.stationClass;
  value.capacity = station.capacity;
  value.enabled = station.enabled;
  impl_->stations.push_back(value);
  std::sort(impl_->stations.begin(), impl_->stations.end(),
            [](const auto &a, const auto &b) { return a.id < b.id; });
}

void FoodServiceSystem::setStaffCapacity(int capacity) {
  if (capacity < 0)
    throw std::invalid_argument("invalid food staff capacity");
  impl_->staffCapacity = capacity;
}

FoodOrderId FoodServiceSystem::createFoodOrder(
    GuestId guest, const MenuOrder &request,
    RoomServiceOrderId roomServiceOrderId) {
  Impl::ActiveOrder order;
  order.id = impl_->nextId++;
  order.guestId = guest;
  order.recipeId = request.recipeId;
  order.channel = request.channel;
  order.quantity = request.quantity;
  order.roomServiceOrderId = roomServiceOrderId;
  order.history.push_back(FoodStage::Queued);

  const auto recipe = impl_->recipes.find(request.recipeId);
  if (recipe == impl_->recipes.end()) {
    impl_->enter(order, FoodStage::Blocked, 0,
                 FoodBlockReason::MissingRecipe);
  } else if (request.quantity <= 0 || request.quantity > 1000) {
    impl_->enter(order, FoodStage::Blocked, 0,
                 FoodBlockReason::InvalidOrder);
  } else {
    order.quality = recipe->second.qualityBase;
    if (recipe->second.priceCents >
        std::numeric_limits<std::int64_t>::max() / request.quantity)
      impl_->enter(order, FoodStage::Blocked, 0,
                   FoodBlockReason::InvalidOrder);
    else
      order.priceCents = recipe->second.priceCents * request.quantity;
  }
  impl_->orders.push_back(order);
  return order.id;
}

bool FoodServiceSystem::beginProduction(FoodOrderId id) {
  auto *order = impl_->findOrder(id);
  if (!order)
    return false;
  if (order->stage == FoodStage::Prep || order->stage == FoodStage::Cook ||
      order->stage == FoodStage::Plate || order->stage == FoodStage::Ready ||
      order->stage == FoodStage::Served || order->stage == FoodStage::Paid ||
      order->stage == FoodStage::Completed)
    return true;
  if (order->blockReason == FoodBlockReason::MissingRecipe ||
      order->blockReason == FoodBlockReason::InvalidOrder)
    return false;

  const auto recipeIt = impl_->recipes.find(order->recipeId);
  if (recipeIt == impl_->recipes.end()) {
    impl_->enter(*order, FoodStage::Blocked, 0,
                 FoodBlockReason::MissingRecipe);
    return false;
  }
  const auto &recipe = recipeIt->second;

  Impl::Station *chosen = nullptr;
  bool foundClass = false;
  for (auto &station : impl_->stations) {
    if (station.stationClass != recipe.requiredStation || !station.enabled)
      continue;
    foundClass = true;
    if (station.active < station.capacity) {
      chosen = &station;
      break;
    }
  }
  if (!foundClass) {
    impl_->enter(*order, FoodStage::Blocked, 0,
                 FoodBlockReason::MissingStation);
    return false;
  }
  if (!chosen) {
    impl_->enter(*order, FoodStage::Blocked, 0,
                 FoodBlockReason::StationCapacity);
    return false;
  }
  if (impl_->activeStaff >= impl_->staffCapacity) {
    impl_->enter(*order, FoodStage::Blocked, 0,
                 FoodBlockReason::NoStaffCapacity);
    return false;
  }
  for (const auto &ingredient : recipe.ingredients) {
    const auto required = ingredient.units * order->quantity;
    if (ingredient.units >
            std::numeric_limits<int>::max() / order->quantity ||
        ingredientUnits(ingredient.item) < required) {
      impl_->enter(*order, FoodStage::Blocked, 0,
                   FoodBlockReason::MissingIngredient);
      return false;
    }
  }

  for (const auto &ingredient : recipe.ingredients)
    impl_->inventory[ingredient.item] -=
        ingredient.units * order->quantity;
  ++chosen->active;
  ++impl_->activeStaff;
  order->resourcesClaimed = true;
  order->stationId = chosen->id;
  order->blockReason = FoodBlockReason::None;
  if (recipe.prepSeconds > 0)
    impl_->enter(*order, FoodStage::Prep, recipe.prepSeconds);
  else {
    order->stage = FoodStage::Prep;
    impl_->advanceStage(*order, recipe);
  }
  return true;
}

void FoodServiceSystem::tickSecond() {
  ++impl_->elapsedSeconds;
  for (auto &order : impl_->orders) {
    if (order.stage != FoodStage::Completed &&
        order.stage != FoodStage::Cancelled)
      ++order.ageSeconds;
    if (order.stage != FoodStage::Prep && order.stage != FoodStage::Cook &&
        order.stage != FoodStage::Plate)
      continue;
    if (order.remainingStageSeconds > 0)
      --order.remainingStageSeconds;
    if (order.remainingStageSeconds == 0) {
      const auto recipe = impl_->recipes.find(order.recipeId);
      if (recipe == impl_->recipes.end()) {
        impl_->releaseProduction(order);
        impl_->enter(order, FoodStage::Blocked, 0,
                     FoodBlockReason::MissingRecipe);
      } else {
        impl_->advanceStage(order, recipe->second);
      }
    }
  }
}

void FoodServiceSystem::tickSeconds(std::int64_t seconds) {
  if (seconds < 0 || seconds > 1000000000LL)
    throw std::invalid_argument("invalid food-service tick duration");
  for (std::int64_t i = 0; i < seconds; ++i)
    tickSecond();
}

FoodOrderView FoodServiceSystem::order(FoodOrderId id) const {
  if (const auto *value = impl_->findOrder(id))
    return *value;
  return {};
}

std::vector<FoodStage>
FoodServiceSystem::stageHistory(FoodOrderId id) const {
  if (const auto *value = impl_->findOrder(id))
    return value->history;
  return {};
}

FoodServiceSnapshot FoodServiceSystem::snapshot() const {
  FoodServiceSnapshot result;
  result.elapsedSeconds = impl_->elapsedSeconds;
  result.revenueCents = impl_->revenueCents;
  for (const auto &[item, units] : impl_->inventory)
    result.inventory.push_back({item, units});
  for (const auto &station : impl_->stations)
    result.stations.push_back(station);
  for (const auto &order : impl_->orders)
    result.orders.push_back(order);
  return result;
}

std::vector<PreparedRoomServiceHandoff>
FoodServiceSystem::pendingRoomServiceHandoffs() const {
  std::vector<PreparedRoomServiceHandoff> result;
  for (const auto &order : impl_->orders)
    if (order.stage == FoodStage::Ready && order.roomServiceOrderId != 0 &&
        order.roomServiceTransferCount == 0)
      result.push_back({order.id, order.roomServiceOrderId});
  return result;
}

bool FoodServiceSystem::acknowledgeRoomServiceHandoff(FoodOrderId id) {
  auto *order = impl_->findOrder(id);
  if (!order || order->stage != FoodStage::Ready ||
      order->roomServiceOrderId == 0 || order->roomServiceTransferCount != 0)
    return false;
  order->roomServiceTransferCount = 1;
  impl_->revenueCents += order->priceCents;
  impl_->enter(*order, FoodStage::Completed);
  return true;
}

std::string FoodServiceSystem::save() const {
  std::ostringstream out;
  out << "HHFNB 1\n";
  out << impl_->nextId << ' ' << impl_->elapsedSeconds << ' '
      << impl_->staffCapacity << ' ' << impl_->activeStaff << ' '
      << impl_->revenueCents << '\n';
  out << impl_->recipes.size() << '\n';
  for (const auto &[id, recipe] : impl_->recipes) {
    out << id << ' ' << std::quoted(recipe.name) << ' '
        << ei(recipe.requiredStation) << ' ' << recipe.prepSeconds << ' '
        << recipe.cookSeconds << ' ' << recipe.plateSeconds << ' '
        << recipe.qualityBase << ' ' << recipe.priceCents << ' '
        << recipe.ingredients.size();
    for (const auto &ingredient : recipe.ingredients)
      out << ' ' << std::quoted(ingredient.item) << ' '
          << ingredient.units;
    out << '\n';
  }
  out << impl_->inventory.size() << '\n';
  for (const auto &[item, units] : impl_->inventory)
    out << std::quoted(item) << ' ' << units << '\n';
  out << impl_->stations.size() << '\n';
  for (const auto &station : impl_->stations)
    out << station.id << ' ' << ei(station.stationClass) << ' '
        << station.capacity << ' ' << station.active << ' '
        << station.enabled << '\n';
  out << impl_->orders.size() << '\n';
  for (const auto &order : impl_->orders) {
    out << order.id << ' ' << order.guestId << ' ' << order.recipeId << ' '
        << ei(order.channel) << ' ' << ei(order.stage) << ' '
        << ei(order.blockReason) << ' ' << order.remainingStageSeconds << ' '
        << order.ageSeconds << ' ' << order.quality << ' '
        << order.priceCents << ' ' << order.roomServiceOrderId << ' '
        << order.roomServiceTransferCount << ' ' << order.quantity << ' '
        << order.stationId << ' ' << order.resourcesClaimed << ' '
        << order.history.size();
    for (const auto stage : order.history)
      out << ' ' << ei(stage);
    out << '\n';
  }
  return out.str();
}

FoodServiceSystem FoodServiceSystem::load(std::string_view data) {
  if (data.size() > 16 * 1024 * 1024)
    throw std::invalid_argument("food-service save too large");
  std::istringstream in{std::string(data)};
  std::string magic;
  int version{};
  in >> magic >> version;
  if (!in || magic != "HHFNB" || version != 1)
    throw std::invalid_argument("unsupported food-service save");

  FoodServiceSystem result;
  auto &d = *result.impl_;
  in >> d.nextId >> d.elapsedSeconds >> d.staffCapacity >> d.activeStaff >>
      d.revenueCents;
  if (!in || d.nextId == 0 || d.elapsedSeconds < 0 ||
      d.staffCapacity < 0 || d.activeStaff < 0 ||
      d.activeStaff > d.staffCapacity || d.revenueCents < 0)
    throw std::invalid_argument("invalid food-service state");

  std::size_t count{};
  in >> count;
  if (!in || count > 100000)
    throw std::invalid_argument("invalid recipe count");
  for (std::size_t index = 0; index < count; ++index) {
    Recipe recipe;
    int stationClass{};
    std::size_t ingredientCount{};
    in >> recipe.id >> std::quoted(recipe.name) >> stationClass >>
        recipe.prepSeconds >> recipe.cookSeconds >> recipe.plateSeconds >>
        recipe.qualityBase >> recipe.priceCents >> ingredientCount;
    if (!in || !validStationClass(stationClass) ||
        ingredientCount > 10000)
      throw std::invalid_argument("invalid saved recipe");
    recipe.requiredStation =
        static_cast<FoodStationClass>(stationClass);
    recipe.ingredients.resize(ingredientCount);
    for (auto &ingredient : recipe.ingredients)
      in >> std::quoted(ingredient.item) >> ingredient.units;
    result.addRecipe(recipe);
  }

  in >> count;
  if (!in || count > 100000)
    throw std::invalid_argument("invalid ingredient inventory count");
  for (std::size_t index = 0; index < count; ++index) {
    std::string item;
    int units{};
    in >> std::quoted(item) >> units;
    result.setIngredientStock(std::move(item), units);
  }

  in >> count;
  if (!in || count > 100000)
    throw std::invalid_argument("invalid station count");
  for (std::size_t index = 0; index < count; ++index) {
    Impl::Station station;
    int stationClass{};
    in >> station.id >> stationClass >> station.capacity >> station.active >>
        station.enabled;
    if (!in || station.id == 0 || !validStationClass(stationClass) ||
        station.capacity <= 0 || station.active < 0 ||
        station.active > station.capacity)
      throw std::invalid_argument("invalid saved station");
    station.stationClass =
        static_cast<FoodStationClass>(stationClass);
    d.stations.push_back(station);
  }
  std::sort(d.stations.begin(), d.stations.end(),
            [](const auto &a, const auto &b) { return a.id < b.id; });

  in >> count;
  if (!in || count > 100000)
    throw std::invalid_argument("invalid food-order count");
  d.orders.resize(count);
  int computedActiveStaff{};
  std::map<FoodStationId, int> computedStationActive;
  for (auto &order : d.orders) {
    int channel{}, stage{}, block{};
    std::size_t historyCount{};
    in >> order.id >> order.guestId >> order.recipeId >> channel >> stage >>
        block >> order.remainingStageSeconds >> order.ageSeconds >>
        order.quality >> order.priceCents >> order.roomServiceOrderId >>
        order.roomServiceTransferCount >> order.quantity >> order.stationId >>
        order.resourcesClaimed >> historyCount;
    if (!in || order.id == 0 || !validChannel(channel) ||
        !validStage(stage) || !validBlock(block) ||
        order.remainingStageSeconds < 0 || order.ageSeconds < 0 ||
        order.quality < 0 || order.quality > 10000 ||
        order.priceCents < 0 || order.roomServiceTransferCount < 0 ||
        order.roomServiceTransferCount > 1 || order.quantity <= 0 ||
        historyCount > 100000)
      throw std::invalid_argument("invalid saved food order");
    order.channel = static_cast<FoodOrderChannel>(channel);
    order.stage = static_cast<FoodStage>(stage);
    order.blockReason = static_cast<FoodBlockReason>(block);
    order.history.resize(historyCount);
    for (auto &historyStage : order.history) {
      int value{};
      in >> value;
      if (!in || !validStage(value))
        throw std::invalid_argument("invalid saved food stage history");
      historyStage = static_cast<FoodStage>(value);
    }
    if (order.resourcesClaimed) {
      ++computedActiveStaff;
      ++computedStationActive[order.stationId];
    }
  }

  if (computedActiveStaff != d.activeStaff)
    throw std::invalid_argument("invalid saved food staff claims");
  for (const auto &station : d.stations)
    if (computedStationActive[station.id] != station.active)
      throw std::invalid_argument("invalid saved station claims");

  in >> std::ws;
  if (!in.eof())
    throw std::invalid_argument("unexpected food-service trailing data");
  return result;
}

} // namespace hh::game
