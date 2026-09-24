#include "hh/game/Simulation.h"
#include "hh/game/BuildJobs.h"
#include "hh/game/BuildingSystems.h"
#include "hh/game/Construction.h"
#include "hh/assets/Json.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <limits>
#include <optional>
#include <queue>
#include <random>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace hh::game {
namespace {
constexpr int MaxMapWidth = 512;
constexpr int MaxMapHeight = 512;
constexpr int MaxMapFloors = 64;

bool same(Position a, Position b) {
  return a.floor == b.floor && a.x == b.x && a.y == b.y;
}
int manhattan(Position a, Position b) {
  return std::abs(a.x - b.x) + std::abs(a.y - b.y) +
         std::abs(a.floor - b.floor) * 8;
}
template <class E> int ei(E e) { return static_cast<int>(e); }
StaffRole staffRole(PersonKind kind) {
  switch (kind) {
  case PersonKind::Receptionist:
    return StaffRole::Receptionist;
  case PersonKind::Maintenance:
    return StaffRole::Maintenance;
  case PersonKind::Housekeeper:
  case PersonKind::Guest:
    return StaffRole::Housekeeper;
  }
  return StaffRole::Housekeeper;
}
PersonKind personKind(StaffRole role) {
  switch (role) {
  case StaffRole::Receptionist:
    return PersonKind::Receptionist;
  case StaffRole::Maintenance:
    return PersonKind::Maintenance;
  case StaffRole::Housekeeper:
    return PersonKind::Housekeeper;
  }
  return PersonKind::Housekeeper;
}
bool passableKind(TileKind kind) {
  return kind == TileKind::Floor || kind == TileKind::Door ||
         kind == TileKind::Entrance || kind == TileKind::FrontDesk ||
         kind == TileKind::SupplyCloset || kind == TileKind::Stairs ||
         kind == TileKind::Bathroom || kind == TileKind::StaffRoom ||
         kind == TileKind::Lobby;
}

[[nodiscard]] std::optional<std::size_t>
checkedTileCount(int width, int height, int floors) noexcept {
  if (width <= 0 || height <= 0 || floors <= 0)
    return std::nullopt;

  constexpr auto MaxIndexableTiles =
      static_cast<std::size_t>(std::numeric_limits<int>::max());
  std::size_t count = static_cast<std::size_t>(width);
  const auto heightSize = static_cast<std::size_t>(height);
  const auto floorSize = static_cast<std::size_t>(floors);
  if (heightSize > MaxIndexableTiles / count)
    return std::nullopt;
  count *= heightSize;
  if (floorSize > MaxIndexableTiles / count)
    return std::nullopt;
  count *= floorSize;
  return count;
}
struct Room : RoomView {};
struct Person : PersonView {
  EntityId reservation{};
  EntityId task{};
  std::int64_t accruedWageUnits{};
  std::int64_t shiftWorkedSeconds{};
  std::int64_t shiftInstanceKey{std::numeric_limits<std::int64_t>::min()};
  bool breakTaskCreated{};
};
struct Reservation : ReservationView {
  double satisfaction{70};
  double checkoutCleanliness{100};
  bool arrived{};
};
struct Task : TaskView {
  double total{};
  bool resourcesClaimed{};
  double trainingSkillGain{};
};
struct PendingOrder : SupplyOrderView {};
bool materialsZero(const ConstructionMaterials &materials) {
  return materials == ConstructionMaterials{};
}
} // namespace

struct Simulation::Impl {
  static constexpr std::size_t completedTaskHistoryLimit = 128;

  struct FootprintBounds {
    int left{};
    int top{};
    int rightExclusive{};
    int bottomExclusive{};
  };

  std::uint64_t seed{1};
  std::mt19937_64 rng{1};
  int width{32}, height{20}, floors{1};
  std::int64_t elapsed{}, remainderMillis{};
  EntityId nextId{1};
  std::vector<TileKind> map;
  std::vector<Room> rooms;
  std::vector<Person> people;
  std::vector<Reservation> reservations;
  std::vector<Reservation> completedReservationHistory;
  std::vector<Task> tasks;
  std::vector<Task> completedTaskHistory;
  std::vector<ReviewView> reviews;
  std::vector<PendingOrder> orders;
  std::vector<ManagerAssignment> managers;
  ConstructionSnapshot construction;
  BuildingSystemsSnapshot buildingSystems;
  InventoryView inventory{24, 48, 36, 24, 8};
  ServiceLogisticsRuntime services{1};
  std::vector<RoomId> managedHousekeepingScratch;
  std::vector<RoomId> workingHousekeepingScratch;
  std::vector<AssetId> managedEngineeringScratch;
  std::vector<AssetId> workingEngineeringScratch;
  FoodServiceSystem food;
  EventsSystem events;
  AmenitiesSystem amenities;
  EconomyView economy{2500000};
  double baseDemand{1.5}, turnoverWork{2400}, repairWork{1800},
      checkInWork{300}, hungerRate{10.0 / 60.0}, restLoss{5.0 / 60.0},
      roomConditionLossPerDay{2.5};
  int utilityPerRoomDayCents{350};
  int onboardingCostCents{0};
  int staffBreakAfterMinutes{0};
  int staffBreakDurationMinutes{30};
  double missedBreakFatiguePerHour{0};
  double missedBreakMoralePerHour{0};
  double trainingSkillGain{5};
  int consumedApplicantDay{-1};
  std::vector<ApplicantId> consumedApplicantIds;
  bool staffOptimizerEnabled{true};

  void configureTutorialFinal05() {
    food.addRecipe({1, "Classic Breakfast", {{"eggs", 2}, {"bread", 2}},
                    FoodStationClass::Oven, 30, 120, 20, 8500, 1800});
    food.addRecipe({2, "Grilled Dinner", {{"protein", 1}, {"produce", 2}},
                    FoodStationClass::Range, 60, 240, 30, 8800, 3200});
    food.addRecipe({3, "House Cocktail", {{"beverage_base", 1}, {"garnish", 1}},
                    FoodStationClass::Bar, 45, 0, 10, 8400, 1600});
    food.setIngredientStock("eggs", 200);
    food.setIngredientStock("bread", 200);
    food.setIngredientStock("protein", 100);
    food.setIngredientStock("produce", 200);
    food.setIngredientStock("beverage_base", 100);
    food.setIngredientStock("garnish", 100);
    food.setStaffCapacity(8);
    food.addStation({1, FoodStationClass::Oven, 3, true});
    food.addStation({2, FoodStationClass::Range, 4, true});
    food.addStation({3, FoodStationClass::Bar, 3, true});
    food.addStation({4, FoodStationClass::Plating, 4, true});
    food.addStation({5, FoodStationClass::ServicePass, 4, true});
    food.addVenue({101, FoodOrderChannel::Breakfast, 48, 360, 630, 20, 45, 30, 20, true});
    food.addVenue({102, FoodOrderChannel::Restaurant, 64, 660, 1380, 20, 60, 30, 30, true});
    food.addVenue({103, FoodOrderChannel::Bar, 32, 960, 120, 15, 30, 20, 20, true});
    events.addFunctionSpace({301, 200, true});
    events.setFoodServiceCapacity(120);
    events.setStaffCapacity(24);
    amenities.addAmenity({201, AmenityType::Gym, 300, 1380, 24, 0, 10000, 10000, 0, 3600, true});
    amenities.addAmenity({202, AmenityType::Spa, 540, 1260, 4, 4, 10000, 10000, 15000, 3600, true});
    amenities.addAmenity({203, AmenityType::Pool, 420, 1320, 36, 2, 10000, 10000, 0, 3600, true});
    food.setElapsedSeconds(elapsed);
    events.setElapsedSeconds(elapsed);
    amenities.setElapsedSeconds(elapsed);
  }

  [[nodiscard]] std::int64_t final05RevenueCents() const {
    return food.snapshot().revenueCents + events.snapshot().revenueCents +
           amenities.snapshot().revenueCents;
  }

  int index(Position p) const { return (p.floor * height + p.y) * width + p.x; }
  bool inside(Position p) const {
    return p.floor >= 0 && p.floor < floors && p.x >= 0 && p.x < width &&
           p.y >= 0 && p.y < height;
  }
  bool passable(Position p) const {
    if (!inside(p))
      return false;
    return passableKind(map[index(p)]);
  }
  std::vector<Position> neighbors(Position p) const {
    std::vector<Position> n{{p.floor, p.x + 1, p.y},
                            {p.floor, p.x - 1, p.y},
                            {p.floor, p.x, p.y + 1},
                            {p.floor, p.x, p.y - 1}};
    if (inside(p) && map[index(p)] == TileKind::Stairs) {
      n.push_back({p.floor + 1, p.x, p.y});
      n.push_back({p.floor - 1, p.x, p.y});
    }
    n.erase(std::remove_if(n.begin(), n.end(),
                           [&](auto q) {
                             return !passable(q) ||
                                    (q.floor != p.floor &&
                                     map[index(q)] != TileKind::Stairs);
                           }),
            n.end());
    return n;
  }
  std::vector<Position> path(Position from, Position to) const {
    if (!passable(from) || !passable(to))
      return {};
    std::vector<int> prev(map.size(), -1);
    std::queue<Position> q;
    q.push(from);
    prev[index(from)] = index(from);
    while (!q.empty()) {
      auto p = q.front();
      q.pop();
      if (same(p, to))
        break;
      for (auto n : neighbors(p))
        if (prev[index(n)] < 0) {
          prev[index(n)] = index(p);
          q.push(n);
        }
    }
    if (prev[index(to)] < 0)
      return {};
    std::vector<Position> out;
    int cur = index(to), start = index(from);
    while (cur != start) {
      int z = cur / (width * height), rem = cur % (width * height);
      out.push_back({z, rem % width, rem / width});
      cur = prev[cur];
    }
    std::reverse(out.begin(), out.end());
    return out;
  }

  [[nodiscard]] std::vector<bool> reachableMask(Position from) const {
    std::vector<bool> reachable(map.size(), false);
    if (!passable(from))
      return reachable;
    std::queue<Position> pending;
    pending.push(from);
    reachable[static_cast<std::size_t>(index(from))] = true;
    while (!pending.empty()) {
      const auto current = pending.front();
      pending.pop();
      for (const auto next : neighbors(current)) {
        const auto nextIndex = static_cast<std::size_t>(index(next));
        if (!reachable[nextIndex]) {
          reachable[nextIndex] = true;
          pending.push(next);
        }
      }
    }
    return reachable;
  }

  Room *getRoom(EntityId id) {
    for (auto &x : rooms)
      if (x.id == id)
        return &x;
    return nullptr;
  }
  Person *getPerson(EntityId id) {
    for (auto &x : people)
      if (x.id == id)
        return &x;
    return nullptr;
  }
  Reservation *getReservation(EntityId id) {
    for (auto &x : reservations)
      if (x.id == id)
        return &x;
    return nullptr;
  }
  const Room *getRoom(EntityId id) const {
    for (const auto &x : rooms)
      if (x.id == id)
        return &x;
    return nullptr;
  }
  BuildJobSnapshot *getBuildJob(EntityId id) {
    for (auto &job : construction.buildJobs)
      if (job.id == id)
        return &job;
    return nullptr;
  }
  BuildJobSnapshot *getBuildJobForTask(EntityId taskId) {
    for (auto &job : construction.buildJobs)
      if (job.taskId == taskId)
        return &job;
    return nullptr;
  }
  ElevatorSnapshot *getElevator(EntityId id) {
    for (auto &elevator : buildingSystems.elevators)
      if (elevator.id == id)
        return &elevator;
    return nullptr;
  }
  RoomSystemSnapshot *getRoomSystem(EntityId roomId) {
    for (auto &system : buildingSystems.rooms)
      if (system.roomId == roomId)
        return &system;
    return nullptr;
  }
  Position locate(TileKind kind) const {
    for (int i = 0; i < (int)map.size(); ++i)
      if (map[i] == kind)
        return {i / (width * height), i % width, (i / width) % height};
    return {-1, -1, -1};
  }
  Position entrance() const { return locate(TileKind::Entrance); }
  Position supply() const { return locate(TileKind::SupplyCloset); }
  Position frontDesk() const { return locate(TileKind::FrontDesk); }
  bool has(TileKind kind) const {
    return std::find(map.begin(), map.end(), kind) != map.end();
  }

  [[nodiscard]] std::optional<FootprintBounds>
  roomFootprintBounds(int floor, int x, int y, int roomWidth,
                      int roomHeight) const noexcept {
    if (floor < 0 || floor >= floors || roomWidth <= 0 || roomHeight <= 0 ||
        x < 0 || y < 0)
      return std::nullopt;
    const auto right = static_cast<std::int64_t>(x) + roomWidth;
    const auto bottom = static_cast<std::int64_t>(y) + roomHeight;
    if (right > width || bottom > height)
      return std::nullopt;
    return FootprintBounds{x, y, static_cast<int>(right),
                           static_cast<int>(bottom)};
  }

  [[nodiscard]] std::optional<std::int64_t>
  furnishedRoomCost(int roomWidth, int roomHeight) const noexcept {
    if (roomWidth <= 0 || roomHeight <= 0)
      return std::nullopt;
    const auto area =
        static_cast<std::int64_t>(roomWidth) * static_cast<std::int64_t>(roomHeight);
    constexpr std::int64_t CostPerTileCents = 15000;
    if (area > std::numeric_limits<std::int64_t>::max() / CostPerTileCents)
      return std::nullopt;
    return area * CostPerTileCents;
  }

  [[nodiscard]] CommandResult validateBuildTile(Position p, TileKind k) const {
    if (!inside(p) || ei(k) < ei(TileKind::Empty) || ei(k) > ei(TileKind::Lobby))
      return {false, "Tile or type is invalid"};
    const auto old = map[index(p)];
    if (old == TileKind::Entrance && k != TileKind::Entrance)
      return {false, "The hotel entrance cannot be removed"};
    if (k == TileKind::Entrance && old != TileKind::Entrance &&
        has(TileKind::Entrance))
      return {false, "The hotel already has a main entrance"};
    if (old == TileKind::FrontDesk && k != TileKind::FrontDesk &&
        std::any_of(people.begin(), people.end(), [](const auto &person) {
          return person.kind == PersonKind::Guest &&
                 person.state != PersonState::CheckedOut;
        }))
      return {false, "Reception is required while guests are on property"};
    if (old == TileKind::SupplyCloset && k != TileKind::SupplyCloset &&
        std::any_of(tasks.begin(), tasks.end(), [](const auto &task) {
          return task.kind == TaskKind::Turnover && !task.resourcesClaimed &&
                 task.status != TaskStatus::Completed;
        }))
      return {false, "Supply closet is required by an active turnover"};
    if (!passableKind(k) &&
        std::any_of(people.begin(), people.end(),
                    [&](const auto &person) { return same(person.position, p); }))
      return {false, "A person is standing on this tile"};
    for (const auto &room : rooms)
      if (p.floor == room.floor && p.x >= room.x && p.x < room.x + room.width &&
          p.y >= room.y && p.y < room.y + room.height)
        return {false, "Use room commands to alter a room"};
    if (old == k)
      return {true, "Tile unchanged"};
    if (economy.cashCents < 500)
      return {false, "Insufficient cash for construction"};
    return {true, "Tile placement valid"};
  }

  [[nodiscard]] CommandResult
  validateBuildFurnishedRoom(const RoomBlueprint &b) const {
    if (b.width < 3 || b.height < 3 || b.beds < 1 || b.baths < 1 ||
        !std::isfinite(b.nightlyRate) || b.nightlyRate <= 0 ||
        b.nightlyRate > 5000)
      return {false,
              "Room requires a 3x3 footprint, bed, bath, and positive rate"};

    const auto bounds =
        roomFootprintBounds(b.floor, b.x, b.y, b.width, b.height);
    if (!bounds)
      return {false, "Room footprint outside property"};

    const auto doorX = static_cast<std::int64_t>(b.door.x);
    const auto doorY = static_cast<std::int64_t>(b.door.y);
    if (b.door.floor != b.floor || doorX < bounds->left ||
        doorX >= bounds->rightExclusive || doorY < bounds->top ||
        doorY >= bounds->bottomExclusive ||
        (doorX != bounds->left && doorX != bounds->rightExclusive - 1 &&
         doorY != bounds->top && doorY != bounds->bottomExclusive - 1))
      return {false, "Door must lie on room perimeter"};

    for (const auto &room : rooms) {
      const auto roomRight =
          static_cast<std::int64_t>(room.x) + room.width;
      const auto roomBottom =
          static_cast<std::int64_t>(room.y) + room.height;
      if (room.floor == b.floor && bounds->left < roomRight &&
          bounds->rightExclusive > room.x && bounds->top < roomBottom &&
          bounds->bottomExclusive > room.y)
        return {false, "Room overlaps another room"};
    }

    for (int y = bounds->top; y < bounds->bottomExclusive; ++y)
      for (int x = bounds->left; x < bounds->rightExclusive; ++x)
        if (map[index({b.floor, x, y})] != TileKind::Empty)
          return {false, "Room footprint contains existing construction"};

    const auto cost = furnishedRoomCost(b.width, b.height);
    if (!cost)
      return {false, "Room construction cost exceeds supported range"};
    if (economy.cashCents < *cost)
      return {false, "Insufficient cash for furnished room construction"};
    return {true, "Furnished room placement valid"};
  }

  void refreshReachability() {
    const auto start = entrance();
    const auto reachable = reachableMask(start);
    for (auto &r : rooms) {
      r.reachable = inside(r.door) &&
                    reachable[static_cast<std::size_t>(index(r.door))] &&
                    !same(start, r.door);
      if (!r.closed && r.status == RoomStatus::Incomplete && r.reachable)
        r.status = RoomStatus::VacantReady;
    }
  }
  [[nodiscard]] bool hasActiveTask(TaskKind kind, EntityId target) const {
    return std::any_of(tasks.begin(), tasks.end(), [&](const auto &task) {
      return task.targetId == target && task.kind == kind &&
             task.status != TaskStatus::Completed;
    });
  }

  EntityId createTask(TaskKind kind, EntityId target, Position pos, double work) {
    for (auto &existing : tasks)
      if (existing.targetId == target && existing.kind == kind &&
          existing.status != TaskStatus::Completed)
        return existing.id;
    Task t;
    t.id = nextId++;
    t.kind = kind;
    t.targetId = target;
    t.target = pos;
    t.total = t.workRemainingSeconds = work;
    tasks.push_back(t);
    return t.id;
  }

  [[nodiscard]] bool createRoomTask(TaskKind kind, EntityId target,
                                    Position pos, double work) {
    if (kind != TaskKind::Turnover && kind != TaskKind::Repair)
      return false;

    // An existing physical task already represents this lifecycle. Do not
    // create a second FINAL-04 job merely because its faster service pipeline
    // happened to finish first.
    if (hasActiveTask(kind, target))
      return true;

    auto stagedServices = services;
    const bool serviceAccepted =
        kind == TaskKind::Turnover
            ? stagedServices.requestRoomTurn(target) != 0
            : stagedServices.createWorkOrder(target, WorkOrderType::Corrective) !=
                  0;
    if (!serviceAccepted)
      return false;

    services = std::move(stagedServices);
    createTask(kind, target, pos, work);
    return true;
  }
  bool objectRemovedBy(const ConstructionObjectView &object,
                       const ConstructionCommand &command) const {
    return std::find(command.removeObjectIds.begin(),
                     command.removeObjectIds.end(),
                     object.id) != command.removeObjectIds.end();
  }
  bool objectOccupies(Position position, const ConstructionCommand &command) const {
    for (const auto &object : construction.objects) {
      if (objectRemovedBy(object, command))
        continue;
      const auto *definition = detail::constructionDefinition(object.typeId);
      if (!definition)
        continue;
      ConstructionPlacement placed{object.typeId, object.origin,
                                   object.rotationQuarterTurns};
      for (const auto cell : detail::constructionFootprint(placed, *definition))
        if (same(cell, position))
          return true;
    }
    return false;
  }
  bool reservedByOtherBuild(Position position, EntityId ignoredJob) const {
    for (const auto &job : construction.buildJobs) {
      if (job.id == ignoredJob || job.state == BuildJobState::Completed ||
          job.state == BuildJobState::Cancelled)
        continue;
      for (const auto &placement : job.construction.placements) {
        const auto *definition = detail::constructionDefinition(placement.typeId);
        if (!definition)
          continue;
        for (const auto cell :
             detail::constructionFootprint(placement, *definition))
          if (same(cell, position))
            return true;
      }
    }
    return false;
  }
  ConstructionPreview validateConstruction(const ConstructionCommand &command,
                                             EntityId ignoredJob = 0,
                                             bool checkCash = true) const {
    ConstructionPreview result;
    if (command.placements.empty() && command.removeObjectIds.empty()) {
      result.reason = ConstructionReason::InvalidCommand;
      result.message = "Construction command is empty";
      return result;
    }
    std::unordered_set<EntityId> removalIds;
    for (const auto id : command.removeObjectIds) {
      if (!removalIds.insert(id).second) {
        result.reason = ConstructionReason::InvalidCommand;
        result.message = "Construction removal list contains duplicates";
        return result;
      }
      const auto found = std::find_if(
          construction.objects.begin(), construction.objects.end(),
          [&](const ConstructionObjectView &object) { return object.id == id; });
      if (found == construction.objects.end()) {
        result.reason = ConstructionReason::ObjectNotFound;
        result.message = "Construction object does not exist";
        return result;
      }
    }

    std::unordered_set<int> commandCells;
    std::int64_t cost = 0;
    for (const auto &placement : command.placements) {
      const auto *definition = detail::constructionDefinition(placement.typeId);
      if (!definition) {
        result.reason = ConstructionReason::UnknownType;
        result.message = "Unknown construction object type";
        return result;
      }
      if (definition->costCents < 0 ||
          cost > std::numeric_limits<std::int64_t>::max() -
                     definition->costCents) {
        result.reason = ConstructionReason::InvalidCommand;
        result.message = "Construction cost is invalid";
        return result;
      }
      cost += definition->costCents;
      const auto footprint = detail::constructionFootprint(placement, *definition);
      for (const auto cell : footprint) {
        if (!inside(cell)) {
          result.reason = ConstructionReason::OutsideProperty;
          result.message = "Object footprint is outside the property";
          return result;
        }
        const auto support = map[index(cell)];
        const bool supported =
            definition->support == ConstructionSupport::Wall
                ? support == TileKind::Wall
                : passableKind(support);
        if (!supported) {
          result.reason = ConstructionReason::InvalidSupport;
          result.message = "Object requires a different structural support";
          return result;
        }
        for (const auto &room : rooms)
          if ((room.status == RoomStatus::Occupied ||
               room.status == RoomStatus::Reserved) &&
              cell.floor == room.floor && cell.x >= room.x &&
              cell.x < room.x + room.width && cell.y >= room.y &&
              cell.y < room.y + room.height) {
            result.reason = ConstructionReason::RoomOccupied;
            result.message = "Occupied or reserved room cannot be altered";
            return result;
          }
        if (objectOccupies(cell, command) ||
            reservedByOtherBuild(cell, ignoredJob) ||
            !commandCells.insert(index(cell)).second) {
          result.reason = ConstructionReason::OccupiedFootprint;
          result.message = "Object footprint is already occupied";
          return result;
        }
        if (definition->blocksMovement &&
            std::any_of(people.begin(), people.end(), [&](const Person &person) {
              return same(person.position, cell);
            })) {
          result.reason = ConstructionReason::AccessObstructed;
          result.message = "A person is standing in the object footprint";
          return result;
        }
      }
      if (definition->requiresAccess) {
        bool accessible = false;
        for (const auto cell : footprint) {
          constexpr std::array<std::array<int, 2>, 4> offsets{{
              {{1, 0}}, {{-1, 0}}, {{0, 1}}, {{0, -1}}}};
          for (const auto offset : offsets) {
            const Position adjacent{cell.floor, cell.x + offset[0],
                                    cell.y + offset[1]};
            if (inside(adjacent) && passable(adjacent) &&
                !objectOccupies(adjacent, command))
              accessible = true;
          }
        }
        if (!accessible) {
          result.reason = ConstructionReason::AccessObstructed;
          result.message = "Object has no usable access edge";
          return result;
        }
      }
    }
    result.costCents = cost;
    if (checkCash && economy.cashCents < cost) {
      result.reason = ConstructionReason::InsufficientCash;
      result.message = "Insufficient cash for construction";
      return result;
    }
    result.valid = true;
    result.reason = ConstructionReason::None;
    result.message = "Construction placement is valid";
    return result;
  }
  ConstructionResult commitConstruction(const ConstructionCommand &command,
                                        bool chargeCash,
                                        EntityId ignoredJob = 0) {
    const auto preview = validateConstruction(command, ignoredJob, chargeCash);
    if (!preview.valid)
      return {false, preview.reason, preview.costCents, preview.message, {}};
    for (const auto id : command.removeObjectIds)
      construction.objects.erase(
          std::remove_if(construction.objects.begin(), construction.objects.end(),
                         [&](const ConstructionObjectView &object) {
                           return object.id == id;
                         }),
          construction.objects.end());
    ConstructionResult result;
    result.ok = true;
    result.costCents = preview.costCents;
    result.message = "Construction committed";
    for (const auto &placement : command.placements) {
      const auto *definition = detail::constructionDefinition(placement.typeId);
      const int rotation =
          detail::normalizedQuarterTurns(placement.rotationQuarterTurns);
      ConstructionObjectView object;
      object.id = nextId++;
      object.typeId = placement.typeId;
      object.origin = placement.origin;
      object.rotationQuarterTurns = rotation;
      object.width = rotation % 2 == 0 ? definition->width : definition->height;
      object.height = rotation % 2 == 0 ? definition->height : definition->width;
      object.blocksMovement = definition->blocksMovement;
      construction.objects.push_back(object);
      result.objectIds.push_back(object.id);
    }
    if (chargeCash) {
      economy.cashCents -= preview.costCents;
      economy.constructionCostCents += preview.costCents;
    }
    return result;
  }

  UtilityNodeSnapshot *utilitySource(UtilityKind kind) {
    for (auto &node : buildingSystems.utilityNodes)
      if (node.kind == kind && node.source)
        return &node;
    return nullptr;
  }
  UtilityNodeSnapshot *roomUtilityNode(EntityId roomId, UtilityKind kind) {
    for (auto &node : buildingSystems.utilityNodes)
      if (node.kind == kind && !node.source && node.roomId == roomId)
        return &node;
    return nullptr;
  }
  UtilityNodeSnapshot &ensureUtilitySource(UtilityKind kind) {
    if (auto *source = utilitySource(kind))
      return *source;
    UtilityNodeSnapshot node;
    node.id = nextId++;
    node.kind = kind;
    node.source = true;
    node.capacity = 1'000'000;
    buildingSystems.utilityNodes.push_back(node);
    return buildingSystems.utilityNodes.back();
  }
  RoomSystemSnapshot &ensureRoomSystem(EntityId roomId) {
    if (auto *system = getRoomSystem(roomId))
      return *system;
    RoomSystemSnapshot system;
    system.roomId = roomId;
    buildingSystems.rooms.push_back(system);
    return buildingSystems.rooms.back();
  }
  void setRoomUtilityInternal(EntityId roomId, UtilityKind kind,
                              bool connected) {
    ensureRoomSystem(roomId);
    auto *node = roomUtilityNode(roomId, kind);
    if (!node) {
      UtilityNodeSnapshot created;
      created.id = nextId++;
      created.kind = kind;
      created.roomId = roomId;
      created.load = 1;
      buildingSystems.utilityNodes.push_back(created);
      node = &buildingSystems.utilityNodes.back();
    }
    const EntityId roomNodeId = node->id;
    buildingSystems.utilityEdges.erase(
        std::remove_if(buildingSystems.utilityEdges.begin(),
                       buildingSystems.utilityEdges.end(),
                       [&](const UtilityEdgeSnapshot &edge) {
                         return edge.from == roomNodeId || edge.to == roomNodeId;
                       }),
        buildingSystems.utilityEdges.end());
    if (connected) {
      const EntityId sourceId = ensureUtilitySource(kind).id;
      buildingSystems.utilityEdges.push_back({sourceId, roomNodeId});
    }
    detail::refreshRoomUtilityFlags(buildingSystems);
  }
  void initializeLegacyRoomSystems(EntityId roomId) {
    auto &system = ensureRoomSystem(roomId);
    system.egress = true;
    system.accessible = true;
    setRoomUtilityInternal(roomId, UtilityKind::Power, true);
    setRoomUtilityInternal(roomId, UtilityKind::Water, true);
  }
  void removeRoomSystems(EntityId roomId) {
    std::unordered_set<EntityId> removedNodes;
    for (const auto &node : buildingSystems.utilityNodes)
      if (!node.source && node.roomId == roomId)
        removedNodes.insert(node.id);
    buildingSystems.utilityEdges.erase(
        std::remove_if(buildingSystems.utilityEdges.begin(),
                       buildingSystems.utilityEdges.end(),
                       [&](const UtilityEdgeSnapshot &edge) {
                         return removedNodes.contains(edge.from) ||
                                removedNodes.contains(edge.to);
                       }),
        buildingSystems.utilityEdges.end());
    buildingSystems.utilityNodes.erase(
        std::remove_if(buildingSystems.utilityNodes.begin(),
                       buildingSystems.utilityNodes.end(),
                       [&](const UtilityNodeSnapshot &node) {
                         return !node.source && node.roomId == roomId;
                       }),
        buildingSystems.utilityNodes.end());
    buildingSystems.rooms.erase(
        std::remove_if(buildingSystems.rooms.begin(), buildingSystems.rooms.end(),
                       [&](const RoomSystemSnapshot &room) {
                         return room.roomId == roomId;
                       }),
        buildingSystems.rooms.end());
    detail::refreshRoomUtilityFlags(buildingSystems);
  }

  Position buildWorkTarget(const BuildJobSnapshot &job) const {
    std::vector<Position> footprint;
    for (const auto &placement : job.construction.placements) {
      const auto *definition = detail::constructionDefinition(placement.typeId);
      if (!definition)
        continue;
      const auto cells = detail::constructionFootprint(placement, *definition);
      footprint.insert(footprint.end(), cells.begin(), cells.end());
    }
    for (const auto id : job.construction.removeObjectIds) {
      const auto object = std::find_if(
          construction.objects.begin(), construction.objects.end(),
          [&](const ConstructionObjectView &candidate) { return candidate.id == id; });
      if (object == construction.objects.end())
        continue;
      const auto *definition = detail::constructionDefinition(object->typeId);
      if (!definition)
        continue;
      ConstructionPlacement placement{object->typeId, object->origin,
                                      object->rotationQuarterTurns};
      const auto cells = detail::constructionFootprint(placement, *definition);
      footprint.insert(footprint.end(), cells.begin(), cells.end());
    }
    if (footprint.empty())
      return entrance();

    Position best{-1, -1, -1};
    constexpr std::array<std::array<int, 2>, 4> offsets{{
        {{1, 0}}, {{-1, 0}}, {{0, 1}}, {{0, -1}}}};
    for (const auto cell : footprint)
      for (const auto offset : offsets) {
        const Position candidate{cell.floor, cell.x + offset[0],
                                 cell.y + offset[1]};
        if (!inside(candidate) || !passable(candidate) ||
            std::find(footprint.begin(), footprint.end(), candidate) !=
                footprint.end() ||
            objectOccupies(candidate, job.construction) ||
            reservedByOtherBuild(candidate, job.id))
          continue;
        if (best.floor < 0 || candidate.floor < best.floor ||
            (candidate.floor == best.floor && candidate.y < best.y) ||
            (candidate.floor == best.floor && candidate.y == best.y &&
             candidate.x < best.x))
          best = candidate;
      }
    return best;
  }

  void tryReserveBuildMaterials(BuildJobSnapshot &job) {
    if (job.state != BuildJobState::WaitingForMaterials &&
        job.state != BuildJobState::Blocked)
      return;
    if (job.materialsConsumed)
      return;
    const Position target = buildWorkTarget(job);
    if (!inside(target)) {
      job.state = BuildJobState::Blocked;
      job.blockedReason = "Build site has no usable work edge";
      return;
    }
    const auto required = detail::constructionMaterialsFor(job.construction);
    if (!detail::hasMaterials(construction.availableMaterials, required)) {
      job.state = BuildJobState::WaitingForMaterials;
      job.blockedReason.clear();
      return;
    }
    detail::subtractMaterials(construction.availableMaterials, required);
    detail::addMaterials(construction.reservedMaterials, required);
    job.reservedMaterials = required;
    job.state = BuildJobState::ReadyForLabor;
    job.blockedReason.clear();
    job.taskId = createTask(TaskKind::Build, job.id, target, job.workSeconds);
  }
  void refreshBuildJobs() {
    for (auto &job : construction.buildJobs)
      tryReserveBuildMaterials(job);
  }
  void markBuildStarted(Task &task) {
    if (task.kind != TaskKind::Build)
      return;
    if (auto *job = getBuildJobForTask(task.id)) {
      if (!job->materialsConsumed) {
        detail::subtractMaterials(construction.reservedMaterials,
                                  job->reservedMaterials);
        job->materialsConsumed = true;
      }
      job->state = BuildJobState::Building;
    }
  }
  void completeBuildJob(Task &task) {
    auto *job = getBuildJobForTask(task.id);
    if (!job)
      return;
    const auto result = commitConstruction(job->construction, false, job->id);
    if (!result.ok) {
      job->state = BuildJobState::Blocked;
      job->blockedReason = result.message;
      return;
    }
    job->state = BuildJobState::Completed;
    job->blockedReason.clear();
  }

  void hourlyBookings(int day, int hour) {
    if (!has(TileKind::Entrance) || !has(TileKind::FrontDesk))
      return;
    std::vector<Room *> free;
    for (auto &r : rooms)
      if (!r.closed && r.status == RoomStatus::VacantReady && r.reachable)
        free.push_back(&r);
    std::shuffle(free.begin(), free.end(), rng);
    const double reputationUtility =
        0.2 + 0.8 * std::clamp((economy.reputation - 60.0) / 20.0, 0.0, 1.0);
    for (Room *r : free) {
      const double priceRatio = r->nightlyRateCents / 16000.0;
      double priceUtility = 0;
      if (priceRatio <= 0.75)
        priceUtility = 1;
      else if (priceRatio <= 1.0)
        priceUtility = 1.0 - (priceRatio - 0.75) * 0.8;
      else if (priceRatio <= 1.25)
        priceUtility = 0.8 - (priceRatio - 1.0) * 2.2;
      else if (priceRatio <= 1.5)
        priceUtility = 0.25 - (priceRatio - 1.25);
      const double dailyChance =
          std::clamp(baseDemand * reputationUtility * priceUtility, 0.0, 1.0);
      const double hourlyChance =
          dailyChance >= 1.0 ? 1.0
                             : 1.0 - std::pow(1.0 - dailyChance, 1.0 / 24.0);
      if (std::generate_canonical<double, 32>(rng) > hourlyChance)
        continue;
      Reservation z;
      z.id = nextId++;
      z.guestName = "Guest " + std::to_string(z.id);
      z.roomId = r->id;
      z.arrivalDay = day + (hour > 15 ? 1 : 0);
      z.departureDay = z.arrivalDay + 1 + (int)(rng() % 3);
      z.nightlyRateCents = r->nightlyRateCents;
      z.nightlyRate = z.nightlyRateCents / 100.0;
      reservations.push_back(z);
      r->status = RoomStatus::Reserved;
      r->reservationId = z.id;
    }
  }
  void arrivals(int day) {
    if (!has(TileKind::FrontDesk))
      return;
    for (auto &z : reservations)
      if (!z.arrived && !z.completed && z.arrivalDay <= day) {
        auto *r = getRoom(z.roomId);
        if (!r || r->status == RoomStatus::OutOfOrder)
          continue;
        z.arrived = true;
        Person p;
        p.id = nextId++;
        p.name = z.guestName;
        p.kind = PersonKind::Guest;
        p.position = entrance();
        p.destination = frontDesk();
        p.state = PersonState::Traveling;
        p.satisfaction = z.satisfaction;
        p.hunger = 90;
        p.rest = 90;
        p.patience = 100;
        p.goal = "Reach front desk";
        p.reservation = z.id;
        people.push_back(p);
      }
  }
  void beginDepartures(int day) {
    if (!has(TileKind::FrontDesk))
      return;
    for (auto &z : reservations)
      if (z.checkedIn && !z.checkoutStarted && !z.completed &&
          z.departureDay <= day) {
        auto *r = getRoom(z.roomId);
        z.checkoutStarted = true;
        z.checkoutCleanliness = r ? r->cleanliness : 80;
        if (r) {
          r->cleanliness = 25;
          r->reservationId = 0;
          if (r->condition < 35) {
            if (!createRoomTask(TaskKind::Repair, r->id, r->door, repairWork))
              throw std::logic_error(
                  "failed to mirror checkout repair into FINAL-04");
            r->status = RoomStatus::OutOfOrder;
          } else {
            if (!createRoomTask(TaskKind::Turnover, r->id, r->door,
                                turnoverWork))
              throw std::logic_error(
                  "failed to mirror checkout turnover into FINAL-04");
            r->status = RoomStatus::VacantDirty;
          }
        }
        for (auto &p : people)
          if (p.reservation == z.id) {
            p.destination = frontDesk();
            p.state = PersonState::Traveling;
            p.goal = "Reach front desk for checkout";
          }
      }
  }
  void completeCheckout(Person &guest) {
    auto *z = getReservation(guest.reservation);
    if (!z || z->completed)
      return;
    auto *r = getRoom(z->roomId);
    z->completed = true;
    economy.completedStays++;
    const auto charge = z->nightlyRateCents * (z->departureDay - z->arrivalDay);
    economy.revenueCents += charge;
    economy.cashCents += charge;
    const int score =
        static_cast<int>(std::clamp(z->satisfaction + z->checkoutCleanliness -
                                        80 - ((r && !r->reachable) ? 20 : 0),
                                    0.0, 100.0));
    reviews.push_back({z->id, static_cast<int>(elapsed / 86400), score,
                       score >= 80   ? "A comfortable, well-run stay."
                       : score >= 60 ? "Fine, though service could improve."
                                     : "Service delays hurt the stay."});
    economy.reputation = economy.reputation * 0.85 + score * 0.15;
    guest.destination = entrance();
    guest.state = PersonState::Traveling;
    guest.goal = "Leave hotel";
  }
  bool shiftActive(const Person &p, int hour) const {
    if (p.shiftStartHour == p.shiftEndHour)
      return true;
    if (p.shiftStartHour < p.shiftEndHour)
      return hour >= p.shiftStartHour && hour < p.shiftEndHour;
    return hour >= p.shiftStartHour || hour < p.shiftEndHour;
  }
  std::int64_t shiftInstanceKey(const Person &p, int day, int hour) const {
    if (p.shiftStartHour == p.shiftEndHour)
      return static_cast<std::int64_t>(day) * 24 + p.shiftStartHour;
    int startDay = day;
    if (p.shiftStartHour > p.shiftEndHour && hour < p.shiftEndHour)
      --startDay;
    return static_cast<std::int64_t>(startDay) * 24 + p.shiftStartHour;
  }
  bool eligible(const Person &p, const Task &task) const {
    if (task.kind == TaskKind::Break || task.kind == TaskKind::Training)
      return p.id == task.targetId;
    if (task.kind == TaskKind::Turnover || task.kind == TaskKind::Restock)
      return p.kind == PersonKind::Housekeeper;
    if (task.kind == TaskKind::Repair || task.kind == TaskKind::Build)
      return p.kind == PersonKind::Maintenance;
    return p.kind == PersonKind::Receptionist;
  }
  void postAccruedWage(Person &person) {
    const std::int64_t cents = person.accruedWageUnits / 3600;
    person.accruedWageUnits %= 3600;
    economy.payrollCents += cents;
    economy.cashCents -= cents;
  }
  void staffAndTasks() {
    const int hour = static_cast<int>((elapsed / 3600) % 24);
    const int day = static_cast<int>(elapsed / 86400);
    std::vector<EntityId> failedRooms;

    for (auto &p : people) {
      if (p.kind == PersonKind::Guest)
        continue;

      const bool scheduled = shiftActive(p, hour);
      if (scheduled) {
        const auto key = shiftInstanceKey(p, day, hour);
        if (p.shiftInstanceKey != key) {
          for (auto &task : tasks)
            if (task.kind == TaskKind::Break && task.targetId == p.id &&
                task.status != TaskStatus::Completed) {
              task.employeeId = 0;
              task.status = TaskStatus::Completed;
            }
          p.shiftInstanceKey = key;
          p.shiftWorkedSeconds = 0;
          p.breakMinutesTakenToday = 0;
          p.breakTaskCreated = false;
          p.onBreak = false;
          p.inTraining = false;
          p.absent = Workforce::absentForShift(seed, p.id, key, p.reliability);
        }

        p.onShift = !p.absent;
        if (!p.onShift) {
          p.state = PersonState::OffDuty;
          p.onBreak = false;
          p.inTraining = false;
          p.task = 0;
          continue;
        }

        p.accruedWageUnits += p.hourlyWageCents;
        if (p.state == PersonState::OffDuty)
          p.state = PersonState::Idle;
        if (!p.onBreak && !p.inTraining)
          ++p.shiftWorkedSeconds;

        const int breakDueSeconds = staffBreakAfterMinutes * 60;
        if (staffBreakAfterMinutes > 0 &&
            p.shiftWorkedSeconds >= breakDueSeconds &&
            p.breakMinutesTakenToday == 0 && !p.breakTaskCreated) {
          Task task;
          task.id = nextId++;
          task.kind = TaskKind::Break;
          task.targetId = p.id;
          task.target = p.position;
          task.total = task.workRemainingSeconds =
              static_cast<double>(staffBreakDurationMinutes * 60);
          task.notBeforeSecond = elapsed;
          tasks.push_back(task);
          p.breakTaskCreated = true;
        }
        if (staffBreakAfterMinutes > 0 &&
            p.shiftWorkedSeconds > breakDueSeconds &&
            p.breakMinutesTakenToday == 0 && !p.onBreak) {
          p.fatigue = std::min(
              100.0, p.fatigue + missedBreakFatiguePerHour / 3600.0);
          p.morale = std::max(
              0.0, p.morale - missedBreakMoralePerHour / 3600.0);
        }
      } else {
        p.onShift = false;
        p.absent = false;
        p.onBreak = false;
        p.inTraining = false;
        p.state = PersonState::OffDuty;
        p.fatigue = std::max(0.0, p.fatigue - 12.0 / 3600.0);
        p.task = 0;
      }
    }

    auto assignReady = [&](int priority) {
      for (auto &task : tasks) {
        const int taskPriority = task.kind == TaskKind::Break
                                     ? 0
                                     : task.kind == TaskKind::Training ? 1 : 2;
        if (taskPriority != priority || elapsed < task.notBeforeSecond ||
            (task.status != TaskStatus::Ready &&
             task.status != TaskStatus::Blocked))
          continue;

        if (task.kind == TaskKind::Break) {
          auto *owner = getPerson(task.targetId);
          if (!owner || !owner->onShift || owner->absent) {
            if (owner)
              owner->breakTaskCreated = false;
            task.employeeId = 0;
            task.status = TaskStatus::Completed;
            continue;
          }
          task.target = owner->position;
        } else if (task.kind == TaskKind::Training) {
          auto *owner = getPerson(task.targetId);
          if (!owner || !owner->onShift || owner->absent)
            continue;
          task.target = owner->position;
        }

        bool resources = true;
        if (task.kind == TaskKind::Turnover)
          resources =
              task.resourcesClaimed ||
              (has(TileKind::SupplyCloset) &&
               services.canClaimRoomTurnSuppliesForSimulation(task.targetId));
        if (task.kind == TaskKind::Repair)
          resources =
              task.resourcesClaimed ||
              services.canClaimCorrectivePartForSimulation(task.targetId);
        if (task.kind == TaskKind::CheckIn || task.kind == TaskKind::CheckOut)
          resources = has(TileKind::FrontDesk);
        if (!resources) {
          task.status = TaskStatus::Blocked;
          task.blockedReason = "Required local supplies unavailable";
          continue;
        }

        task.blockedReason.clear();
        task.status = TaskStatus::Ready;
        Person *best = nullptr;
        int bestDistance = std::numeric_limits<int>::max();
        for (auto &p : people)
          if (p.onShift && !p.absent && p.task == 0 &&
              p.goal != "Preventive maintenance" && eligible(p, task)) {
            const int distance = manhattan(p.position, task.target);
            if (distance < bestDistance ||
                (distance == bestDistance && (!best || p.id < best->id))) {
              bestDistance = distance;
              best = &p;
            }
          }
        if (!best)
          continue;

        if (task.kind == TaskKind::Repair && !task.resourcesClaimed) {
          if (!services.claimCorrectivePartForSimulation(task.targetId)) {
            task.status = TaskStatus::Blocked;
            task.blockedReason = "Canonical maintenance part unavailable";
            continue;
          }
          task.resourcesClaimed = true;
        }

        task.employeeId = best->id;
        best->task = task.id;
        best->destination =
            (task.kind == TaskKind::Turnover && !task.resourcesClaimed)
                ? supply()
                : task.target;
        best->state = PersonState::Traveling;
        task.status = TaskStatus::Traveling;
        if (task.kind == TaskKind::Turnover)
          if (auto *room = getRoom(task.targetId))
            room->status = RoomStatus::Cleaning;
      }
    };

    assignReady(0);
    assignReady(1);
    assignReady(2);

    for (auto &task : tasks) {
      if (!task.employeeId || task.status == TaskStatus::Completed ||
          task.status == TaskStatus::Blocked)
        continue;
      auto *p = getPerson(task.employeeId);
      if (!p || !p->onShift || p->absent) {
        if (p) {
          p->task = 0;
          p->onBreak = false;
          p->inTraining = false;
          p->state = PersonState::OffDuty;
        }
        task.employeeId = 0;
        if (task.kind == TaskKind::Break) {
          task.status = TaskStatus::Completed;
          if (p)
            p->breakTaskCreated = false;
        } else {
          task.status = TaskStatus::Ready;
        }
        continue;
      }

      if (p->state == PersonState::Traveling) {
        if (task.kind != TaskKind::Break && task.kind != TaskKind::Training) {
          p->fatigue = std::min(100.0, p->fatigue + 4.0 / 3600.0);
          ++p->travelSeconds;
        }
        auto route = path(p->position, p->destination);
        if (route.empty() && !same(p->position, p->destination)) {
          task.status = TaskStatus::Ready;
          task.employeeId = 0;
          p->task = 0;
          p->state = PersonState::Idle;
          continue;
        }
        if (!route.empty())
          p->position = route.front();
        if (same(p->position, p->destination)) {
          if (task.kind == TaskKind::Turnover &&
              same(p->destination, supply()) && !task.resourcesClaimed) {
            if (!services.claimRoomTurnSuppliesForSimulation(task.targetId)) {
              task.status = TaskStatus::Blocked;
              task.blockedReason = "Canonical room supplies unavailable";
              task.employeeId = 0;
              p->task = 0;
              p->state = PersonState::Idle;
              continue;
            }
            task.resourcesClaimed = true;
            p->destination = task.target;
            p->state = PersonState::Traveling;
          } else {
            p->state = PersonState::Working;
            task.status = TaskStatus::Working;
            markBuildStarted(task);
          }
        }
        continue;
      }

      if (p->state != PersonState::Working)
        continue;

      if (task.kind == TaskKind::Break) {
        p->onBreak = true;
        p->inTraining = false;
        p->fatigue = std::max(0.0, p->fatigue - 12.0 / 3600.0);
        task.workRemainingSeconds -= 1.0;
      } else if (task.kind == TaskKind::Training) {
        p->onBreak = false;
        p->inTraining = true;
        task.workRemainingSeconds -= 1.0;
        p->trainingProgress =
            task.total > 0
                ? std::clamp(
                      100.0 * (1.0 - task.workRemainingSeconds / task.total),
                      0.0, 100.0)
                : 100.0;
      } else {
        p->onBreak = false;
        p->inTraining = false;
        const double fatiguePerHour =
            task.kind == TaskKind::Turnover
                ? 10.0
                : task.kind == TaskKind::Build
                      ? 8.0
                      : (task.kind == TaskKind::CheckIn ||
                                 task.kind == TaskKind::CheckOut
                             ? 4.0
                             : 6.0);
        p->fatigue =
            std::min(100.0, p->fatigue + fatiguePerHour / 3600.0);
        double efficiency = 0.75 + 0.75 * p->skill / 100.0;
        if (p->fatigue > 80)
          efficiency /= 1.25;
        else if (p->fatigue > 60)
          efficiency /= 1.10;
        task.workRemainingSeconds -= efficiency;
      }

      if (task.workRemainingSeconds > 0)
        continue;

      task.status = TaskStatus::Completed;
      p->task = 0;
      p->state = PersonState::Idle;
      if (task.kind == TaskKind::Break) {
        p->onBreak = false;
        p->breakTaskCreated = false;
        p->breakMinutesTakenToday +=
            static_cast<int>(std::ceil(task.total / 60.0));
        continue;
      }
      if (task.kind == TaskKind::Training) {
        p->inTraining = false;
        p->trainingProgress = 100.0;
        p->skill =
            std::clamp(p->skill + task.trainingSkillGain, 0.0, 100.0);
        continue;
      }
      if (task.kind == TaskKind::CheckIn) {
        if (auto *guest = getPerson(task.targetId)) {
          for (auto &reservation : reservations)
            if (reservation.id == guest->reservation) {
              if (auto *room = getRoom(reservation.roomId)) {
                guest->destination = room->door;
                guest->state = PersonState::Traveling;
                guest->goal = "Reach assigned room";
                reservation.checkedIn = true;
                room->status = RoomStatus::Occupied;
              }
            }
        }
      }
      if (task.kind == TaskKind::CheckOut)
        if (auto *guest = getPerson(task.targetId))
          completeCheckout(*guest);
      if (task.kind == TaskKind::Build)
        completeBuildJob(task);

      if (auto *room = getRoom(task.targetId)) {
        if (task.kind == TaskKind::Turnover && !room->closed) {
          if (room->condition < 35) {
            room->status = RoomStatus::OutOfOrder;
            failedRooms.push_back(room->id);
          } else {
            // Physical labor is complete; FINAL-04 service completion remains
            // authoritative for final sellability.
            room->status = RoomStatus::Cleaning;
            room->cleanliness = std::clamp(
                70 + p->skill * 0.3 - p->fatigue * 0.1, 0.0, 100.0);
          }
        }
        if (task.kind == TaskKind::Repair) {
          // Physical completion does not reset condition. FINAL-04
          // engineering owns the repair state and condition reset.
          if (!room->closed)
            room->status = RoomStatus::OutOfOrder;
        }
      }
    }

    for (EntityId roomId : failedRooms)
      if (auto *room = getRoom(roomId))
        if (!createRoomTask(TaskKind::Repair, roomId, room->door, repairWork))
          throw std::logic_error(
              "failed to mirror turnover failure repair into FINAL-04");

    // Supply labor to authoritative FINAL-04 preventive engineering work
    // without creating a second physical task authority.
    std::unordered_set<EntityId> preventiveWorkers;
    const auto engineering = services.engineering().snapshot();
    for (const auto &order : engineering.workOrders) {
      if (order.type != WorkOrderType::Preventive ||
          order.stage == WorkOrderStage::Completed)
        continue;
      auto *target = getRoom(order.assetId);
      if (!target)
        continue;
      Person *best = nullptr;
      int bestDistance = std::numeric_limits<int>::max();
      for (auto &person : people) {
        if (person.kind != PersonKind::Maintenance || !person.onShift ||
            person.absent || person.task != 0 ||
            preventiveWorkers.contains(person.id))
          continue;
        const int distance = manhattan(person.position, target->door);
        if (distance < bestDistance ||
            (distance == bestDistance && (!best || person.id < best->id))) {
          bestDistance = distance;
          best = &person;
        }
      }
      if (!best)
        continue;
      preventiveWorkers.insert(best->id);
      best->destination = target->door;
      best->goal = "Preventive maintenance";
      if (!same(best->position, best->destination)) {
        auto route = path(best->position, best->destination);
        if (route.empty()) {
          best->state = PersonState::Idle;
          best->goal.clear();
          continue;
        }
        best->state = PersonState::Traveling;
        best->position = route.front();
        ++best->travelSeconds;
        best->fatigue = std::min(100.0, best->fatigue + 4.0 / 3600.0);
        continue;
      }
      const auto serviceWork = services.workEngineeringSecond(
          order.assetId, WorkOrderType::Preventive);
      if (!serviceWork.valid ||
          serviceWork.blockedReason != BlockReason::None) {
        best->state = PersonState::Idle;
        best->goal.clear();
        continue;
      }
      best->state = PersonState::Working;
      best->fatigue = std::min(100.0, best->fatigue + 6.0 / 3600.0);
      if (serviceWork.completed) {
        best->state = PersonState::Idle;
        best->goal.clear();
      }
    }
  }
  void guests() {
    const int hour = static_cast<int>((elapsed / 3600) % 24);
    for (auto &p : people)
      if (p.kind == PersonKind::Guest && p.state != PersonState::CheckedOut) {
        if (p.state == PersonState::Sleeping && hour >= 7 && hour < 22) {
          p.state = PersonState::Idle;
          p.goal = "Relax in room";
        } else if (p.state == PersonState::Idle && (hour >= 22 || hour < 7)) {
          p.state = PersonState::Sleeping;
          p.goal = "Sleep in room";
        }
        if (p.state == PersonState::Sleeping)
          p.rest = std::min(100.0, p.rest + 22.0 / 3600.0);
        else {
          p.hunger = std::max(0.0, p.hunger - hungerRate / 60.0);
          p.rest = std::max(0.0, p.rest - restLoss / 60.0);
        }
        if (p.state == PersonState::Traveling) {
          ++p.travelSeconds;
          auto route = path(p.position, p.destination);
          if (!route.empty())
            p.position = route.front();
          else {
            p.queueWaitSeconds += 1;
            p.patience = std::max(0.0, p.patience - 1.0 / 120);
            if (p.queueWaitSeconds > 8 * 60)
              p.satisfaction = std::max(0.0, p.satisfaction - 0.6 / 60.0);
          }
          if (same(p.position, p.destination)) {
            if (p.goal == "Reach front desk") {
              p.state = PersonState::Waiting;
              p.goal = "Wait for check-in";
              createTask(TaskKind::CheckIn, p.id, frontDesk(), checkInWork);
            } else if (p.goal == "Reach front desk for checkout") {
              p.state = PersonState::Waiting;
              p.goal = "Wait for checkout";
              createTask(TaskKind::CheckOut, p.id, frontDesk(),
                         checkInWork * 0.6);
            } else if (p.goal == "Leave hotel") {
              p.state = PersonState::CheckedOut;
              p.goal = "Departed";
            } else {
              p.state = hour >= 7 && hour < 22 ? PersonState::Idle
                                               : PersonState::Sleeping;
              p.goal = p.state == PersonState::Sleeping ? "Sleep in room"
                                                        : "Relax in room";
            }
          }
        } else if (p.state == PersonState::Waiting) {
          p.queueWaitSeconds += 1;
          p.patience = std::max(0.0, p.patience - 1.0 / 120);
          if (p.queueWaitSeconds > 8 * 60)
            p.satisfaction = std::max(0.0, p.satisfaction - 0.6 / 60.0);
        }
        for (auto &z : reservations)
          if (z.id == p.reservation)
            z.satisfaction = p.satisfaction;
      }
  }
  void compactTransientState() {
    people.erase(std::remove_if(people.begin(), people.end(), [](const auto &p) {
                   return p.kind == PersonKind::Guest &&
                          p.state == PersonState::CheckedOut;
                 }),
                 people.end());

    for (auto &task : tasks)
      if (task.status == TaskStatus::Completed)
        completedTaskHistory.push_back(std::move(task));
    tasks.erase(std::remove_if(tasks.begin(), tasks.end(), [](const auto &task) {
                  return task.status == TaskStatus::Completed;
                }),
                tasks.end());
    if (completedTaskHistory.size() > completedTaskHistoryLimit)
      completedTaskHistory.erase(
          completedTaskHistory.begin(),
          completedTaskHistory.begin() + static_cast<std::ptrdiff_t>(
                                             completedTaskHistory.size() -
                                             completedTaskHistoryLimit));

    for (auto &reservation : reservations)
      if (reservation.completed)
        completedReservationHistory.push_back(std::move(reservation));
    reservations.erase(
        std::remove_if(reservations.begin(), reservations.end(),
                       [](const auto &reservation) {
                         return reservation.completed;
                       }),
        reservations.end());
  }
  void tickServicesWithPhysicalLabor() {
    managedHousekeepingScratch.clear();
    workingHousekeepingScratch.clear();
    managedEngineeringScratch.clear();
    workingEngineeringScratch.clear();

    const auto requiredCapacity = tasks.size();
    if (managedHousekeepingScratch.capacity() < requiredCapacity) {
      managedHousekeepingScratch.reserve(requiredCapacity);
      workingHousekeepingScratch.reserve(requiredCapacity);
      managedEngineeringScratch.reserve(requiredCapacity);
      workingEngineeringScratch.reserve(requiredCapacity);
    }

    for (const auto &task : tasks) {
      if (task.status == TaskStatus::Completed)
        continue;
      if (task.kind == TaskKind::Turnover) {
        managedHousekeepingScratch.push_back(task.targetId);
        if (task.status == TaskStatus::Working)
          workingHousekeepingScratch.push_back(task.targetId);
      } else if (task.kind == TaskKind::Repair) {
        managedEngineeringScratch.push_back(task.targetId);
        if (task.status == TaskStatus::Working)
          workingEngineeringScratch.push_back(task.targetId);
      }
    }

    services.tickSimulationSecond(
        managedHousekeepingScratch, workingHousekeepingScratch,
        managedEngineeringScratch, workingEngineeringScratch);
  }

  void reconcileRoomServiceCompletion() {
    for (auto &room : rooms) {
      if (room.closed || room.reservationId != 0 ||
          room.status == RoomStatus::Occupied ||
          room.status == RoomStatus::Incomplete)
        continue;

      if (room.status == RoomStatus::Cleaning &&
          !hasActiveTask(TaskKind::Turnover, room.id) &&
          services.housekeeping().roomStatus(room.id) ==
              ServiceRoomStatus::Ready) {
        if (room.condition < 35) {
          room.status = RoomStatus::OutOfOrder;
          if (!createRoomTask(TaskKind::Repair, room.id, room.door, repairWork))
            throw std::logic_error(
                "failed to create repair after completed room turn");
        } else {
          room.status = RoomStatus::VacantReady;
        }
      }

      if (room.status == RoomStatus::OutOfOrder &&
          !hasActiveTask(TaskKind::Repair, room.id)) {
        const auto engineeringStage =
            services.engineering().latestWorkOrderStage(
                room.id, WorkOrderType::Corrective);
        if (engineeringStage &&
            *engineeringStage == WorkOrderStage::Completed) {
          room.condition = 100;
          services.synchronizeAssetConditionForSimulation(room.id, 10000, false);
          room.status = RoomStatus::VacantDirty;
          if (!createRoomTask(TaskKind::Turnover, room.id, room.door,
                              turnoverWork))
            throw std::logic_error(
                "failed to create turnover after completed repair");
        }
      }
    }

    int available = 0;
    int occupied = 0;
    for (const auto &room : rooms)
      if (!room.closed && room.status != RoomStatus::Incomplete) {
        ++available;
        occupied += room.status == RoomStatus::Occupied;
      }
    economy.occupancy = available ? double(occupied) / available : 0;
  }

  void minute() {
    elapsed += 1;
    int minute = (elapsed / 60) % 60, hour = (elapsed / 3600) % 24,
        day = elapsed / 86400;
    bool hourBoundary = (elapsed % 3600) == 0;
    if (hourBoundary)
      hourlyBookings(day, hour);
    if (hourBoundary && hour == 11)
      beginDepartures(day);
    if (hourBoundary && hour == 15)
      arrivals(day);
    if (hourBoundary) {
      for (auto &o : orders)
        if (!o.delivered && o.etaDay <= day)
          o.delivered = true;
    }
    refreshBuildJobs();
    staffAndTasks();
    guests();
    for (auto &elevator : buildingSystems.elevators)
      detail::tickElevator(elevator);
    compactTransientState();
    if (hour == 0 && minute == 0 && hourBoundary) {
      for (auto &p : people)
        if (p.kind != PersonKind::Guest)
          postAccruedWage(p);
      const auto util =
          static_cast<std::int64_t>(rooms.size()) * utilityPerRoomDayCents;
      economy.utilityCostCents += util;
      economy.cashCents -= util;
      std::vector<EntityId> newlyFailedRooms;
      for (auto &r : rooms)
        if (r.status != RoomStatus::OutOfOrder) {
          const double wearVariation =
              0.85 + 0.3 * std::generate_canonical<double, 32>(rng);
          r.condition = std::max(0.0, r.condition - roomConditionLossPerDay *
                                                        wearVariation);
          services.synchronizeAssetConditionForSimulation(
              r.id,
              std::clamp(static_cast<int>(std::llround(r.condition * 100.0)),
                         0, 10000),
              r.condition < 35);
          if (r.condition < 35 && r.reservationId == 0 &&
              r.status == RoomStatus::VacantReady) {
            r.status = RoomStatus::OutOfOrder;
            newlyFailedRooms.push_back(r.id);
          }
        }
      for (const EntityId roomId : newlyFailedRooms)
        if (auto *room = getRoom(roomId))
          if (!createRoomTask(TaskKind::Repair, roomId, room->door, repairWork))
            throw std::logic_error(
                "failed to mirror wear failure repair into FINAL-04");
      economy.distressed = economy.cashCents < 0;
      economy.stars = std::min(5, 1 + economy.completedStays / 15);
    }
    int available = 0, occupied = 0;
    for (auto &r : rooms)
      if (!r.closed && r.status != RoomStatus::Incomplete) {
        available++;
        occupied += r.status == RoomStatus::Occupied;
      }
    economy.occupancy = available ? double(occupied) / available : 0;
  }
};

Simulation::Simulation(std::uint64_t seed, int w, int h, int f)
    : impl_(std::make_unique<Impl>()) {
  const auto tileCount = checkedTileCount(w, h, f);
  if (w < 4 || w > MaxMapWidth || h < 4 || h > MaxMapHeight ||
      f < 1 || f > MaxMapFloors || !tileCount)
    throw std::invalid_argument("invalid map dimensions");
  impl_->seed = seed;
  impl_->rng.seed(seed);
  impl_->services = ServiceLogisticsRuntime(seed);
  auto &serviceInventory = impl_->services.logistics();
  const auto seedServiceItem = [&](StorageKind kind, std::string_view item, int quantity) {
    if (quantity > 0 &&
        !serviceInventory.addInventory(serviceInventory.firstStorage(kind), item, quantity))
      throw std::logic_error("failed to seed service inventory");
  };
  seedServiceItem(StorageKind::CleanLinen, "clean_linen_set", impl_->inventory.linen);
  seedServiceItem(StorageKind::FloorCloset, "towel_unit", impl_->inventory.towels);
  seedServiceItem(StorageKind::FloorCloset, "amenity_kit", impl_->inventory.amenities);
  seedServiceItem(StorageKind::FloorCloset, "cleaning_chemical", impl_->inventory.chemicals);
  seedServiceItem(StorageKind::CentralStorage, "maintenance_part", impl_->inventory.parts);
  impl_->width = w;
  impl_->height = h;
  impl_->floors = f;
  impl_->map.assign(*tileCount, TileKind::Empty);
  impl_->economy.reputation = 70;
  impl_->economy.stars = 1;
}
Simulation::~Simulation() = default;
Simulation::Simulation(Simulation &&) noexcept = default;
Simulation &Simulation::operator=(Simulation &&) noexcept = default;
Simulation::Simulation(const Simulation &o)
    : impl_(std::make_unique<Impl>(*o.impl_)) {}
Simulation &Simulation::operator=(const Simulation &o) {
  if (this != &o)
    impl_ = std::make_unique<Impl>(*o.impl_);
  return *this;
}

Simulation Simulation::tutorial(std::uint64_t seed) {
  Simulation s(seed, 32, 20, 3);
  // Scenario seed capital: enough to open the six-room starter hotel with a
  // meaningful expansion reserve after construction is posted.
  s.impl_->economy.cashCents = 5'000'000;
  for (int x = 0; x < 26; ++x)
    s.buildTile({0, x, 8}, x == 0  ? TileKind::Entrance
                           : x < 7 ? TileKind::Lobby
                                   : TileKind::Floor);
  s.buildTile({0, 2, 8}, TileKind::FrontDesk);
  s.buildTile({0, 7, 8}, TileKind::SupplyCloset);
  s.buildTile({0, 12, 8}, TileKind::Stairs);
  s.buildTile({1, 12, 8}, TileKind::Stairs);
  s.buildTile({2, 12, 8}, TileKind::Stairs);
  for (int f = 1; f < 3; ++f)
    for (int x = 2; x < 26; ++x)
      s.buildTile({f, x, 8}, x == 12 ? TileKind::Stairs : TileKind::Floor);
  for (int floor = 0; floor < 2; ++floor)
    for (int i = 0; i < 3; ++i) {
      const int x = 4 + i * 8;
      s.buildFurnishedRoom({std::to_string((floor + 1) * 100 + i + 1),
                            floor,
                            x,
                            9,
                            6,
                            6,
                            {floor, x, 9},
                            1,
                            1,
                            135.0 + i * 10 + floor * 5});
    }
  s.hireStaff({"Alex", PersonKind::Receptionist, 8, 22, 20});
  s.hireStaff({"Morgan", PersonKind::Housekeeper, 8, 16, 18});
  s.hireStaff({"Casey", PersonKind::Maintenance, 10, 12, 25});
  s.impl_->elapsed = 14 * 3600;
  s.impl_->services.synchronizeElapsedSecondsForSimulation(s.impl_->elapsed);
  s.impl_->configureTutorialFinal05();
  return s;
}
CommandResult Simulation::previewBuildTile(Position p, TileKind k) const {
  return impl_->validateBuildTile(p, k);
}

CommandResult
Simulation::previewBuildFurnishedRoom(const RoomBlueprint &b) const {
  return impl_->validateBuildFurnishedRoom(b);
}

CommandResult Simulation::buildTile(Position p, TileKind k) {
  const auto validation = impl_->validateBuildTile(p, k);
  if (!validation.ok)
    return validation;
  if (impl_->map[impl_->index(p)] == k)
    return validation;
  impl_->map[impl_->index(p)] = k;
  impl_->economy.cashCents -= 500;
  impl_->economy.constructionCostCents += 500;
  impl_->refreshReachability();
  return {true, "Tile built"};
}
CommandResult Simulation::buildFurnishedRoom(const RoomBlueprint &b) {
  const auto validation = impl_->validateBuildFurnishedRoom(b);
  if (!validation.ok)
    return validation;
  const auto bounds =
      impl_->roomFootprintBounds(b.floor, b.x, b.y, b.width, b.height);
  const auto cost = impl_->furnishedRoomCost(b.width, b.height);
  if (!bounds || !cost)
    return {false, "Room construction parameters exceed supported range"};
  for (int y = bounds->top; y < bounds->bottomExclusive; ++y)
    for (int x = bounds->left; x < bounds->rightExclusive; ++x) {
      Position p{b.floor, x, y};
      const bool edge = x == bounds->left ||
                        x == bounds->rightExclusive - 1 ||
                        y == bounds->top ||
                        y == bounds->bottomExclusive - 1;
      impl_->map[impl_->index(p)] = same(p, b.door) ? TileKind::Door
                                    : edge          ? TileKind::Wall
                                                    : TileKind::Floor;
    }
  impl_->map[impl_->index({b.floor, b.x + 1, b.y + 1})] = TileKind::Bathroom;
  Room r;
  r.id = impl_->nextId++;
  r.name = b.name;
  r.door = b.door;
  r.floor = b.floor;
  r.x = b.x;
  r.y = b.y;
  r.width = b.width;
  r.height = b.height;
  r.beds = b.beds;
  r.baths = b.baths;
  r.nightlyRateCents =
      static_cast<std::int64_t>(std::llround(b.nightlyRate * 100));
  r.nightlyRate = r.nightlyRateCents / 100.0;
  r.cleanliness = 100;
  r.condition = 100;
  r.reachable = !impl_->path(impl_->entrance(), b.door).empty();
  r.status = r.reachable ? RoomStatus::VacantReady : RoomStatus::Incomplete;
  impl_->rooms.push_back(r);
  impl_->services.registerRoom(
      r.id, r.status == RoomStatus::VacantReady ? ServiceRoomStatus::Ready
                                                 : ServiceRoomStatus::Blocked);
  impl_->services.registerAsset(
      r.id, std::clamp(static_cast<int>(std::llround(r.condition * 100.0)), 0, 10000));
  impl_->economy.cashCents -= *cost;
  impl_->economy.constructionCostCents += *cost;
  return {true,
          r.reachable ? "Furnished room opened"
                      : "Room built but lacks an entrance route",
          r.id};
}
CommandResult Simulation::hireStaff(const StaffHire &h) {
  if (!impl_->has(TileKind::Entrance))
    return {false, "Build an entrance before hiring staff"};
  if (ei(h.role) < ei(PersonKind::Receptionist) ||
      ei(h.role) > ei(PersonKind::Maintenance) || h.name.empty() ||
      !std::isfinite(h.hourlyWage) || h.hourlyWage <= 0 ||
      h.hourlyWage > 10000 || h.shiftStartHour < 0 || h.shiftStartHour > 23 ||
      h.shiftEndHour < 0 || h.shiftEndHour > 23)
    return {false, "Invalid staff details"};
  Person p;
  p.id = impl_->nextId++;
  p.name = h.name;
  p.kind = h.role;
  p.position = impl_->entrance();
  p.destination = p.position;
  p.state = PersonState::OffDuty;
  p.skill = 65;
  p.shiftStartHour = h.shiftStartHour;
  p.shiftEndHour = h.shiftEndHour;
  p.hourlyWageCents =
      static_cast<std::int64_t>(std::llround(h.hourlyWage * 100));
  p.reliability = 100.0;
  p.contract = {0, staffRole(h.role), p.hourlyWageCents, 0,
                h.shiftStartHour, h.shiftEndHour};
  impl_->people.push_back(p);
  return {true, "Staff hired", p.id};
}
std::vector<Applicant> Simulation::applicants() const {
  const int day = static_cast<int>(impl_->elapsed / 86400);
  auto pool = Workforce::applicantPool(impl_->seed, day);
  if (impl_->consumedApplicantDay != day)
    return pool;
  pool.erase(std::remove_if(pool.begin(), pool.end(), [&](const Applicant &a) {
               return std::find(impl_->consumedApplicantIds.begin(),
                                impl_->consumedApplicantIds.end(),
                                a.id) != impl_->consumedApplicantIds.end();
             }),
             pool.end());
  return pool;
}
HireResult Simulation::hireApplicant(ApplicantId applicantId) {
  if (!impl_->has(TileKind::Entrance))
    return {false, "Build an entrance before hiring staff", 0, applicantId};
  const int day = static_cast<int>(impl_->elapsed / 86400);
  if (impl_->consumedApplicantDay != day) {
    impl_->consumedApplicantDay = day;
    impl_->consumedApplicantIds.clear();
  }
  const auto pool = Workforce::applicantPool(impl_->seed, day);
  const auto found = std::find_if(pool.begin(), pool.end(),
                                  [&](const Applicant &a) {
                                    return a.id == applicantId;
                                  });
  if (found == pool.end() ||
      std::find(impl_->consumedApplicantIds.begin(),
                impl_->consumedApplicantIds.end(), applicantId) !=
          impl_->consumedApplicantIds.end())
    return {false, "Applicant is not available", 0, applicantId};
  if (impl_->economy.cashCents < impl_->onboardingCostCents)
    return {false, "Insufficient cash for onboarding", 0, applicantId};

  Person p;
  p.id = impl_->nextId++;
  p.name = found->name;
  p.kind = personKind(found->role);
  p.position = impl_->entrance();
  p.destination = p.position;
  p.state = PersonState::OffDuty;
  p.skill = found->skill;
  p.reliability = found->reliability;
  p.shiftStartHour = found->shiftStartHour;
  p.shiftEndHour = found->shiftEndHour;
  p.hourlyWageCents = found->wageExpectationCents;
  p.contract = {found->id, found->role, found->wageExpectationCents,
                impl_->onboardingCostCents, found->shiftStartHour,
                found->shiftEndHour};
  impl_->people.push_back(p);
  impl_->consumedApplicantIds.push_back(found->id);
  impl_->economy.cashCents -= impl_->onboardingCostCents;
  return {true, "Applicant hired", p.id, found->id};
}
CommandResult Simulation::fireStaff(EntityId id) {
  auto it =
      std::find_if(impl_->people.begin(), impl_->people.end(), [&](auto &p) {
        return p.id == id && p.kind != PersonKind::Guest;
      });
  if (it == impl_->people.end())
    return {false, "Employee not found"};
  impl_->tasks.erase(
      std::remove_if(impl_->tasks.begin(), impl_->tasks.end(),
                     [&](const Task &task) {
                       return task.status != TaskStatus::Completed &&
                              (task.kind == TaskKind::Break ||
                               task.kind == TaskKind::Training) &&
                              task.targetId == id;
                     }),
      impl_->tasks.end());
  for (auto &t : impl_->tasks)
    if (t.employeeId == id && t.status != TaskStatus::Completed) {
      t.employeeId = 0;
      t.status = TaskStatus::Ready;
    }
  impl_->postAccruedWage(*it);
  impl_->people.erase(it);
  impl_->managers.erase(
      std::remove_if(impl_->managers.begin(), impl_->managers.end(),
                     [&](const ManagerAssignment &assignment) {
                       return assignment.managerId == id;
                     }),
      impl_->managers.end());
  return {true, "Staff released", id};
}
CommandResult Simulation::setStaffShift(EntityId id, int a, int b) {
  auto *p = impl_->getPerson(id);
  if (!p || p->kind == PersonKind::Guest || a < 0 || a > 23 || b < 0 || b > 23)
    return {false, "Invalid employee or shift"};
  p->shiftStartHour = a;
  p->shiftEndHour = b;
  p->contract.shiftStartHour = a;
  p->contract.shiftEndHour = b;
  return {true, "Shift updated", id};
}
CommandResult Simulation::scheduleTraining(EntityId id,
                                                   std::int64_t startSecond,
                                                   int durationMinutes) {
  auto *person = impl_->getPerson(id);
  if (!person || person->kind == PersonKind::Guest || durationMinutes <= 0 ||
      durationMinutes > 24 * 60 || startSecond < impl_->elapsed ||
      startSecond > impl_->elapsed + 30LL * 86400LL)
    return {false, "Invalid employee or training window"};
  if (person->task != 0 || person->absent)
    return {false, "Training cannot overlap active work"};
  if (std::any_of(impl_->tasks.begin(), impl_->tasks.end(), [&](const Task &task) {
        return task.kind == TaskKind::Training && task.targetId == id &&
               task.status != TaskStatus::Completed;
      }))
    return {false, "Employee already has scheduled training"};

  Task task;
  task.id = impl_->nextId++;
  task.kind = TaskKind::Training;
  task.targetId = id;
  task.target = person->position;
  task.total = task.workRemainingSeconds =
      static_cast<double>(durationMinutes * 60);
  task.notBeforeSecond = startSecond;
  task.trainingSkillGain = impl_->trainingSkillGain;
  impl_->tasks.push_back(task);
  person->trainingProgress = 0;
  return {true, "Training scheduled", task.id};
}
CommandResult Simulation::assignDepartmentManager(DepartmentId department,
                                                   EntityId employeeId) {
  if (ei(department) < ei(DepartmentId::FrontOffice) ||
      ei(department) > ei(DepartmentId::Engineering))
    return {false, "Invalid department"};
  if (employeeId == 0) {
    impl_->managers.erase(
        std::remove_if(impl_->managers.begin(), impl_->managers.end(),
                       [&](const ManagerAssignment &assignment) {
                         return assignment.department == department;
                       }),
        impl_->managers.end());
    return {true, "Department manager cleared"};
  }
  auto *employee = impl_->getPerson(employeeId);
  if (!employee || employee->kind == PersonKind::Guest ||
      staffRole(employee->kind) != departmentRole(department))
    return {false, "Manager must belong to the department"};

  auto it = std::find_if(impl_->managers.begin(), impl_->managers.end(),
                         [&](const ManagerAssignment &assignment) {
                           return assignment.department == department;
                         });
  if (it == impl_->managers.end())
    impl_->managers.push_back({department, employeeId});
  else
    it->managerId = employeeId;
  return {true, "Department manager assigned", employeeId};
}

std::vector<DepartmentView> Simulation::departments() const {
  std::vector<DepartmentView> result;
  result.reserve(allDepartments().size());
  for (const auto department : allDepartments()) {
    DepartmentView view;
    view.id = department;
    view.name = departmentName(department);
    view.role = departmentRole(department);
    const auto manager =
        std::find_if(impl_->managers.begin(), impl_->managers.end(),
                     [&](const ManagerAssignment &assignment) {
                       return assignment.department == department;
                     });
    if (manager != impl_->managers.end())
      view.managerId = manager->managerId;
    for (const auto &person : impl_->people)
      if (person.kind != PersonKind::Guest &&
          staffRole(person.kind) == view.role && person.id != view.managerId)
        view.directReports.push_back(person.id);
    std::sort(view.directReports.begin(), view.directReports.end());
    result.push_back(std::move(view));
  }
  return result;
}

DepartmentForecast Simulation::departmentForecast(DepartmentId department,
                                                   SimDay day) const {
  if (ei(department) < ei(DepartmentId::FrontOffice) ||
      ei(department) > ei(DepartmentId::Engineering) || day < 0)
    throw std::invalid_argument("invalid department forecast request");

  DepartmentForecast forecast;
  forecast.department = department;
  forecast.day = day;
  for (int bucket = 0; bucket < 4; ++bucket)
    forecast.buckets.push_back(
        {bucket * 360, (bucket + 1) * 360, 0, 0, 0});

  auto addRequired = [&](int minuteOfDay, int minutes) {
    if (minutes <= 0)
      return;
    const int bucket = std::clamp(minuteOfDay / 360, 0, 3);
    forecast.buckets[static_cast<std::size_t>(bucket)].requiredMinutes +=
        minutes;
  };
  auto roundedMinutes = [](double seconds) {
    return static_cast<int>(std::ceil(std::max(0.0, seconds) / 60.0));
  };

  if (department == DepartmentId::Housekeeping) {
    for (const auto &reservation : impl_->reservations)
      if (!reservation.completed && reservation.departureDay == day)
        addRequired(11 * 60, roundedMinutes(impl_->turnoverWork));
    if (day == static_cast<int>(impl_->elapsed / 86400))
      for (const auto &task : impl_->tasks)
        if (task.kind == TaskKind::Turnover &&
            task.status != TaskStatus::Completed)
          addRequired(static_cast<int>((impl_->elapsed / 60) % 1440),
                      roundedMinutes(task.workRemainingSeconds));
  } else if (department == DepartmentId::FrontOffice) {
    for (const auto &reservation : impl_->reservations) {
      if (reservation.completed)
        continue;
      if (reservation.arrivalDay == day && !reservation.checkedIn)
        addRequired(15 * 60, roundedMinutes(impl_->checkInWork));
      if (reservation.departureDay == day && !reservation.checkoutStarted)
        addRequired(11 * 60, roundedMinutes(impl_->checkInWork * 0.6));
    }
  } else {
    if (day == static_cast<int>(impl_->elapsed / 86400)) {
      for (const auto &task : impl_->tasks)
        if (task.kind == TaskKind::Repair &&
            task.status != TaskStatus::Completed)
          addRequired(static_cast<int>((impl_->elapsed / 60) % 1440),
                      roundedMinutes(task.workRemainingSeconds));
    } else {
      double repairSeconds{};
      int repairCount{};
      for (const auto &task : impl_->completedTaskHistory)
        if (task.kind == TaskKind::Repair) {
          repairSeconds += task.total;
          ++repairCount;
        }
      if (repairCount)
        addRequired(10 * 60, roundedMinutes(repairSeconds / repairCount));
    }
  }

  const StaffRole role = departmentRole(department);
  const std::int64_t dayStart = static_cast<std::int64_t>(day) * 86400;
  auto overlapSeconds = [](std::int64_t a0, std::int64_t a1,
                           std::int64_t b0, std::int64_t b1) {
    return std::max<std::int64_t>(0, std::min(a1, b1) - std::max(a0, b0));
  };

  for (const auto &person : impl_->people) {
    if (person.kind == PersonKind::Guest || staffRole(person.kind) != role)
      continue;
    const std::int64_t shiftDuration =
        person.shiftStartHour == person.shiftEndHour
            ? 86400
            : static_cast<std::int64_t>(
                  (person.shiftEndHour - person.shiftStartHour + 24) % 24) *
                  3600;
    for (int startOffset = -1; startOffset <= 0; ++startOffset) {
      const int startDay = day + startOffset;
      const std::int64_t shiftStart =
          static_cast<std::int64_t>(startDay) * 86400 +
          person.shiftStartHour * 3600LL;
      const std::int64_t shiftEnd = shiftStart + shiftDuration;
      const auto key = static_cast<std::int64_t>(startDay) * 24 +
                       person.shiftStartHour;
      if (person.absent && person.shiftInstanceKey == key)
        continue;

      for (std::size_t bucketIndex = 0;
           bucketIndex < forecast.buckets.size(); ++bucketIndex) {
        const auto &bucket = forecast.buckets[bucketIndex];
        const std::int64_t bucketStart = dayStart + bucket.startMinute * 60LL;
        const std::int64_t bucketEnd = dayStart + bucket.endMinute * 60LL;
        forecast.buckets[bucketIndex].scheduledMinutes +=
            static_cast<int>(overlapSeconds(shiftStart, shiftEnd, bucketStart,
                                            bucketEnd) /
                             60);
      }

      if (impl_->staffBreakAfterMinutes > 0) {
        const std::int64_t breakStart =
            shiftStart + impl_->staffBreakAfterMinutes * 60LL;
        const std::int64_t breakEnd =
            std::min<std::int64_t>(
                shiftEnd, breakStart +
                              static_cast<std::int64_t>(
                                  impl_->staffBreakDurationMinutes) *
                                  60);
        if (breakEnd > breakStart)
          for (std::size_t bucketIndex = 0;
               bucketIndex < forecast.buckets.size(); ++bucketIndex) {
            const auto &bucket = forecast.buckets[bucketIndex];
            const std::int64_t bucketStart =
                dayStart + bucket.startMinute * 60LL;
            const std::int64_t bucketEnd = dayStart + bucket.endMinute * 60LL;
            forecast.buckets[bucketIndex].scheduledMinutes -=
                static_cast<int>(overlapSeconds(breakStart, breakEnd,
                                                bucketStart, bucketEnd) /
                                 60);
          }
      }
    }

    for (const auto &task : impl_->tasks) {
      if (task.kind != TaskKind::Training || task.targetId != person.id ||
          task.status == TaskStatus::Completed)
        continue;
      const auto trainingStart = task.notBeforeSecond;
      const auto trainingEnd = trainingStart +
                               static_cast<std::int64_t>(std::ceil(task.total));
      for (std::size_t bucketIndex = 0;
           bucketIndex < forecast.buckets.size(); ++bucketIndex) {
        const auto &bucket = forecast.buckets[bucketIndex];
        const std::int64_t bucketStart = dayStart + bucket.startMinute * 60LL;
        const std::int64_t bucketEnd = dayStart + bucket.endMinute * 60LL;
        forecast.buckets[bucketIndex].scheduledMinutes -=
            static_cast<int>(overlapSeconds(trainingStart, trainingEnd,
                                            bucketStart, bucketEnd) /
                             60);
      }
    }
  }

  for (auto &bucket : forecast.buckets) {
    bucket.scheduledMinutes = std::max(0, bucket.scheduledMinutes);
    bucket.uncoveredMinutes =
        std::max(0, bucket.requiredMinutes - bucket.scheduledMinutes);
    forecast.requiredMinutes += bucket.requiredMinutes;
    forecast.scheduledMinutes += bucket.scheduledMinutes;
    forecast.uncoveredMinutes += bucket.uncoveredMinutes;
  }
  return forecast;
}

OptimizerSnapshot Simulation::buildOptimizerSnapshot() const {
  OptimizerSnapshot snapshot;
  snapshot.capturedSecond = impl_->elapsed;
  snapshot.horizonEndSecond = impl_->elapsed + 86400;
  const int currentDay = static_cast<int>(impl_->elapsed / 86400);

  auto serviceRole = [](TaskKind kind) {
    if (kind == TaskKind::Turnover || kind == TaskKind::Restock)
      return StaffRole::Housekeeper;
    if (kind == TaskKind::Repair || kind == TaskKind::Build)
      return StaffRole::Maintenance;
    return StaffRole::Receptionist;
  };
  auto addWindow = [&](std::vector<OptimizationWindow> &windows,
                       std::int64_t start, std::int64_t end) {
    start = std::max(start, snapshot.capturedSecond);
    end = std::min(end, snapshot.horizonEndSecond);
    if (end > start)
      windows.push_back({start, end});
  };

  for (const auto &person : impl_->people) {
    if (person.kind == PersonKind::Guest)
      continue;
    OptimizerEmployee employee;
    employee.id = person.id;
    employee.role = staffRole(person.kind);
    employee.absent = person.absent;
    employee.availableNow = person.onShift && !person.absent && person.task == 0;

    const std::int64_t shiftDuration =
        person.shiftStartHour == person.shiftEndHour
            ? 86400
            : static_cast<std::int64_t>(
                  (person.shiftEndHour - person.shiftStartHour + 24) % 24) *
                  3600;
    for (int offset = -1; offset <= 2; ++offset) {
      const int startDay = currentDay + offset;
      const std::int64_t shiftStart =
          static_cast<std::int64_t>(startDay) * 86400 +
          person.shiftStartHour * 3600LL;
      const std::int64_t shiftEnd = shiftStart + shiftDuration;
      const std::int64_t key = static_cast<std::int64_t>(startDay) * 24 +
                               person.shiftStartHour;
      if (!(person.absent && person.shiftInstanceKey == key))
        addWindow(employee.shiftWindows, shiftStart, shiftEnd);

      if (impl_->staffBreakAfterMinutes > 0) {
        const std::int64_t breakStart =
            shiftStart + impl_->staffBreakAfterMinutes * 60LL;
        const std::int64_t breakEnd = std::min<std::int64_t>(
            shiftEnd,
            breakStart +
                static_cast<std::int64_t>(impl_->staffBreakDurationMinutes) *
                    60);
        const bool alreadyTookCurrentBreak =
            key == person.shiftInstanceKey && person.breakMinutesTakenToday > 0;
        if (!alreadyTookCurrentBreak)
          addWindow(employee.unavailableWindows, breakStart, breakEnd);
      }
    }

    for (const auto &task : impl_->tasks) {
      if (task.status == TaskStatus::Completed)
        continue;
      if ((task.kind == TaskKind::Break || task.kind == TaskKind::Training) &&
          task.targetId == person.id) {
        const auto start =
            task.status == TaskStatus::Working ? impl_->elapsed
                                               : task.notBeforeSecond;
        const auto duration = static_cast<std::int64_t>(
            std::ceil(task.status == TaskStatus::Working
                          ? task.workRemainingSeconds
                          : task.total));
        addWindow(employee.unavailableWindows, start, start + duration);
      } else if (task.employeeId == person.id &&
                 task.kind != TaskKind::Break &&
                 task.kind != TaskKind::Training) {
        const auto duration = static_cast<std::int64_t>(
            std::ceil(std::max(1.0, task.workRemainingSeconds)));
        addWindow(employee.unavailableWindows, impl_->elapsed,
                  impl_->elapsed + duration);
      }
    }

    std::sort(employee.shiftWindows.begin(), employee.shiftWindows.end(),
              [](const auto &a, const auto &b) {
                if (a.startSecond != b.startSecond)
                  return a.startSecond < b.startSecond;
                return a.endSecond < b.endSecond;
              });
    std::sort(employee.unavailableWindows.begin(),
              employee.unavailableWindows.end(), [](const auto &a, const auto &b) {
                if (a.startSecond != b.startSecond)
                  return a.startSecond < b.startSecond;
                return a.endSecond < b.endSecond;
              });
    snapshot.employees.push_back(std::move(employee));
  }

  for (const auto &task : impl_->tasks) {
    if (task.employeeId != 0 || task.status == TaskStatus::Completed ||
        task.kind == TaskKind::Break || task.kind == TaskKind::Training)
      continue;
    OptimizerTask view;
    view.id = task.id;
    view.requiredRole = serviceRole(task.kind);
    view.critical = task.kind == TaskKind::Repair ||
                    task.kind == TaskKind::CheckIn ||
                    task.kind == TaskKind::CheckOut;
    view.earliestStartSecond =
        std::max(impl_->elapsed, task.notBeforeSecond);
    view.durationSeconds = static_cast<int>(std::clamp<std::int64_t>(
        static_cast<std::int64_t>(
            std::ceil(std::max(1.0, task.workRemainingSeconds))),
        1, 7LL * 86400LL));
    snapshot.tasks.push_back(view);
  }

  std::sort(snapshot.employees.begin(), snapshot.employees.end(),
            [](const auto &a, const auto &b) { return a.id < b.id; });
  std::sort(snapshot.tasks.begin(), snapshot.tasks.end(),
            [](const auto &a, const auto &b) { return a.id < b.id; });
  return snapshot;
}

PlanValidation Simulation::validatePlan(const OptimizerSnapshot &snapshot,
                                        const AssignmentPlan &plan) const {
  return validateAssignmentPlan(snapshot, plan);
}

CommandResult Simulation::setRoomRate(EntityId id, double rate) {
  auto *r = impl_->getRoom(id);
  if (!r || !std::isfinite(rate) || rate < 20 || rate > 5000)
    return {false, "Room or rate invalid"};
  r->nightlyRateCents = static_cast<std::int64_t>(std::llround(rate * 100));
  r->nightlyRate = r->nightlyRateCents / 100.0;
  return {true, "Rate updated", id};
}
CommandResult Simulation::requestClean(EntityId id) {
  auto *r = impl_->getRoom(id);
  if (!r || r->reservationId != 0 || r->status == RoomStatus::Occupied ||
      r->status == RoomStatus::OutOfOrder || r->condition < 40)
    return {false, "Occupied, reserved, or closed room cannot be cleaned"};
  if (!impl_->createRoomTask(TaskKind::Turnover, id, r->door,
                             impl_->turnoverWork))
    return {false, "Housekeeping service could not accept the room turn"};
  r->status = RoomStatus::VacantDirty;
  return {true, "Cleaning requested", id};
}
CommandResult Simulation::requestRepair(EntityId id) {
  auto *r = impl_->getRoom(id);
  if (!r || r->status == RoomStatus::Occupied || r->reservationId != 0)
    return {false, "Occupied or reserved room cannot be repaired"};
  if (std::any_of(impl_->tasks.begin(), impl_->tasks.end(), [&](auto &task) {
        return task.targetId == id && task.status != TaskStatus::Completed;
      }))
    return {false, "Complete existing room service before requesting repair"};
  impl_->services.synchronizeAssetConditionForSimulation(
      id,
      std::clamp(static_cast<int>(std::llround(r->condition * 100.0)), 0,
                 10000),
      r->condition < 35);
  if (!impl_->createRoomTask(TaskKind::Repair, id, r->door,
                             impl_->repairWork))
    return {false, "Engineering service could not accept the repair"};
  r->status = RoomStatus::OutOfOrder;
  return {true, "Repair requested", id};
}
CommandResult Simulation::closeRoom(EntityId id, bool closed) {
  auto *r = impl_->getRoom(id);
  if (!r || r->status == RoomStatus::Occupied || r->reservationId != 0)
    return {false, "Occupied or reserved room cannot change availability"};
  if (!closed) {
    const bool activeRepair =
        std::any_of(impl_->tasks.begin(), impl_->tasks.end(), [&](auto &task) {
          return task.targetId == id && task.kind == TaskKind::Repair &&
                 task.status != TaskStatus::Completed;
        });
    if ((!r->closed && r->status == RoomStatus::OutOfOrder) || activeRepair ||
        r->condition < 40)
      return {false, "Room must be repaired before reopening"};
  }
  r->closed = closed;
  r->status = closed ? RoomStatus::OutOfOrder
                     : (r->cleanliness >= 70 ? RoomStatus::VacantReady
                                             : RoomStatus::VacantDirty);
  return {true, closed ? "Room closed" : "Room reopened", id};
}
CommandResult Simulation::removeRoom(EntityId id) {
  auto it = std::find_if(impl_->rooms.begin(), impl_->rooms.end(),
                         [&](auto &r) { return r.id == id; });
  if (it == impl_->rooms.end() || it->status == RoomStatus::Occupied ||
      it->reservationId)
    return {false, "Room is occupied or reserved"};
  if (std::any_of(impl_->tasks.begin(), impl_->tasks.end(), [&](auto &task) {
        return task.targetId == id && task.status != TaskStatus::Completed;
      }))
    return {false, "Complete room service before demolition"};
  if (!impl_->services.retireRoomAndAsset(id))
    return {false, "Complete housekeeping or engineering work before demolition"};
  for (int y = it->y; y < it->y + it->height; ++y)
    for (int x = it->x; x < it->x + it->width; ++x)
      impl_->map[impl_->index({it->floor, x, y})] = TileKind::Empty;
  impl_->removeRoomSystems(id);
  impl_->rooms.erase(it);
  impl_->refreshReachability();
  return {true, "Room removed", id};
}
CommandResult Simulation::orderSupplies(const SupplyOrder &o) {
  const auto valid = [](int quantity) {
    return quantity >= 0 && quantity <= 100000;
  };
  const std::int64_t total = static_cast<std::int64_t>(o.linen) + o.towels +
                             o.amenities + o.chemicals + o.parts;
  if (!valid(o.linen) || !valid(o.towels) || !valid(o.amenities) ||
      !valid(o.chemicals) || !valid(o.parts) || total == 0)
    return {false, "Order quantities invalid"};
  std::int64_t cost = static_cast<std::int64_t>(o.linen) * 1200 +
                      static_cast<std::int64_t>(o.towels) * 500 +
                      static_cast<std::int64_t>(o.amenities) * 250 +
                      static_cast<std::int64_t>(o.chemicals) * 800 +
                      static_cast<std::int64_t>(o.parts) * 3500;
  if (impl_->economy.cashCents < cost)
    return {false, "Insufficient cash"};

  // Stage FINAL-04 logistics on a copy first. The legacy order remains in
  // place for the physical scheduler until that subsystem is retired, but a
  // player purchase must be visible to both authorities or the operations UI
  // and usable service inventory diverge immediately.
  auto stagedServices = impl_->services;
  auto &logistics = stagedServices.logistics();
  const int etaDay = static_cast<int>(impl_->elapsed / 86400) + 2;
  const auto secondsUntilEta =
      std::max<std::int64_t>(0, static_cast<std::int64_t>(etaDay) * 86400 -
                                    impl_->elapsed);
  const int leadSeconds = static_cast<int>(
      std::min<std::int64_t>(secondsUntilEta, std::numeric_limits<int>::max()));

  const auto stageItem = [&](std::string_view item, int quantity,
                             StorageKind destinationKind) {
    if (quantity == 0)
      return true;
    const auto destination = logistics.firstStorage(destinationKind);
    if (destination == 0)
      return false;
    return logistics
        .placePurchaseOrder(item, quantity, destination, leadSeconds)
        .ok();
  };

  if (!stageItem("clean_linen_set", o.linen, StorageKind::CleanLinen) ||
      !stageItem("towel_unit", o.towels, StorageKind::CentralStorage) ||
      !stageItem("amenity_kit", o.amenities, StorageKind::CentralStorage) ||
      !stageItem("cleaning_chemical", o.chemicals, StorageKind::CentralStorage) ||
      !stageItem("maintenance_part", o.parts, StorageKind::CentralStorage))
    return {false, "Service storage cannot accept this supply order"};

  PendingOrder p;
  p.id = impl_->nextId++;
  p.items = {o.linen, o.towels, o.amenities, o.chemicals, o.parts};
  p.etaDay = etaDay;
  impl_->services = std::move(stagedServices);
  impl_->orders.push_back(p);
  impl_->economy.cashCents -= cost;
  impl_->economy.supplyCostCents += cost;
  return {true, "Order submitted", p.id};
}
CommandResult Simulation::loadDefinitions(std::string_view j) {
  auto d = *impl_;
  hh::assets::JsonValue root;
  try {
    root = hh::assets::parse_json(j);
  } catch (const std::exception &) {
    return {false, "Definitions must be valid JSON"};
  }
  if (!root.is_object())
    return {false, "Definitions must be a JSON object"};
  const bool initialLinenOverride = root.find("initialLinen") != nullptr;
  const bool initialTowelsOverride = root.find("initialTowels") != nullptr;
  const bool initialAmenitiesOverride = root.find("initialAmenities") != nullptr;
  const bool initialChemicalsOverride = root.find("initialChemicals") != nullptr;
  const bool initialPartsOverride = root.find("initialParts") != nullptr;
  const bool inventoryOverride =
      initialLinenOverride || initialTowelsOverride ||
      initialAmenitiesOverride || initialChemicalsOverride ||
      initialPartsOverride;
  auto number = [&](std::string_view key, double &out) {
    const auto *value = root.find(key);
    if (!value)
      return true;
    if (!value->is_number())
      return false;
    out = value->as_number();
    return std::isfinite(out);
  };
  auto integer = [&](std::string_view key, int &out) {
    double x = out;
    if (!number(key, x) || x < 0 || x > 100000 || std::floor(x) != x)
      return false;
    out = (int)x;
    return true;
  };
  if (!number("baseDemand", d.baseDemand) ||
      !integer("utilityPerRoomDayCents", d.utilityPerRoomDayCents) ||
      !number("turnoverWorkSeconds", d.turnoverWork) ||
      !number("repairWorkSeconds", d.repairWork) ||
      !number("checkInWorkSeconds", d.checkInWork) ||
      !number("guestHungerPerMinute", d.hungerRate) ||
      !number("guestRestLossPerMinute", d.restLoss) ||
      !number("roomConditionLossPerDay", d.roomConditionLossPerDay) ||
      !integer("onboardingCostCents", d.onboardingCostCents) ||
      !integer("staffBreakAfterMinutes", d.staffBreakAfterMinutes) ||
      !integer("staffBreakDurationMinutes", d.staffBreakDurationMinutes) ||
      !number("missedBreakFatiguePerHour", d.missedBreakFatiguePerHour) ||
      !number("missedBreakMoralePerHour", d.missedBreakMoralePerHour) ||
      !number("trainingSkillGain", d.trainingSkillGain) ||
      !integer("initialLinen", d.inventory.linen) ||
      !integer("initialTowels", d.inventory.towels) ||
      !integer("initialAmenities", d.inventory.amenities) ||
      !integer("initialChemicals", d.inventory.chemicals) ||
      !integer("initialParts", d.inventory.parts) || d.baseDemand < 0 ||
      d.turnoverWork <= 0 || d.repairWork <= 0 || d.checkInWork <= 0 ||
      d.roomConditionLossPerDay < 0 || d.roomConditionLossPerDay > 100 ||
      d.staffBreakDurationMinutes <= 0 ||
      d.staffBreakDurationMinutes > 24 * 60 ||
      d.missedBreakFatiguePerHour < 0 ||
      d.missedBreakFatiguePerHour > 1000 ||
      d.missedBreakMoralePerHour < 0 ||
      d.missedBreakMoralePerHour > 1000 ||
      d.trainingSkillGain < 0 || d.trainingSkillGain > 100)
    return {false, "Definition values are invalid"};

  if (inventoryOverride) {
    const auto logisticsState = d.services.logisticsSnapshot();
    const bool activePurchase = std::any_of(
        logisticsState.purchaseOrders.begin(), logisticsState.purchaseOrders.end(),
        [](const auto &order) {
          return order.state != PurchaseOrderState::Completed &&
                 order.state != PurchaseOrderState::RejectedNoCapacity &&
                 order.state != PurchaseOrderState::Cancelled;
        });
    const bool activeMove = std::any_of(
        logisticsState.stockMoves.begin(), logisticsState.stockMoves.end(),
        [](const auto &move) { return !move.completed; });
    if (activePurchase || activeMove)
      return {false,
              "Initial inventory overrides require idle service logistics"};

    auto &serviceInventory = d.services.logistics();
    const auto syncItem = [&](bool requested, std::string_view item, int target,
                              StorageKind preferredStorage) {
      if (!requested)
        return true;
      const int current = serviceInventory.inventoryUsable(item);
      if (serviceInventory.totalInventory(item) != current)
        return false;
      if (current > target)
        return serviceInventory.consumeUsable(item, current - target);
      if (current == target)
        return true;

      const int additional = target - current;
      if (serviceInventory.addToKind(preferredStorage, item, additional))
        return true;

      // Definition files describe starting scenario state, not a live
      // procurement action. Preserve legacy scenarios with intentionally large
      // starting stock by provisioning explicit overflow storage rather than
      // clipping stock or rejecting an otherwise valid definition.
      const auto overflow =
          serviceInventory.addStorage({preferredStorage, additional, true});
      return overflow != 0 &&
             serviceInventory.addInventory(overflow, item, additional);
    };

    if (!syncItem(initialLinenOverride, "clean_linen_set", d.inventory.linen,
                  StorageKind::CleanLinen) ||
        !syncItem(initialTowelsOverride, "towel_unit", d.inventory.towels,
                  StorageKind::CentralStorage) ||
        !syncItem(initialAmenitiesOverride, "amenity_kit",
                  d.inventory.amenities, StorageKind::CentralStorage) ||
        !syncItem(initialChemicalsOverride, "cleaning_chemical",
                  d.inventory.chemicals, StorageKind::CentralStorage) ||
        !syncItem(initialPartsOverride, "maintenance_part", d.inventory.parts,
                  StorageKind::CentralStorage))
      return {false, "Initial inventory could not be synchronized"};
  }

  *impl_ = std::move(d);
  return {true, "Definitions loaded"};
}
void Simulation::setStaffOptimizerEnabled(bool enabled) {
  impl_->staffOptimizerEnabled = enabled;
}

ConstructionPreview
Simulation::previewConstruction(const ConstructionCommand &command) const {
  return impl_->validateConstruction(command);
}
ConstructionResult
Simulation::executeConstruction(const ConstructionCommand &command) {
  return impl_->commitConstruction(command, true);
}
BuildQueueResult Simulation::queueBuild(const BuildPlan &plan) {
  if (plan.workSeconds <= 0 || plan.workSeconds > 7 * 86400)
    return {false, 0, ConstructionReason::InvalidCommand,
            "Build work duration is invalid"};
  const auto preview = impl_->validateConstruction(plan.construction);
  if (!preview.valid)
    return {false, 0, preview.reason, preview.message};
  BuildJobSnapshot job;
  job.id = impl_->nextId++;
  job.construction = plan.construction;
  job.reservedCashCents = preview.costCents;
  job.workSeconds = plan.workSeconds;
  impl_->economy.cashCents -= preview.costCents;
  impl_->economy.constructionCostCents += preview.costCents;
  impl_->construction.buildJobs.push_back(job);
  impl_->tryReserveBuildMaterials(impl_->construction.buildJobs.back());
  return {true, job.id, ConstructionReason::None, "Build job queued"};
}
CommandResult Simulation::cancelBuild(EntityId jobId) {
  auto *job = impl_->getBuildJob(jobId);
  if (!job || job->state == BuildJobState::Completed ||
      job->state == BuildJobState::Cancelled)
    return {false, "Build job cannot be cancelled"};
  if (job->taskId) {
    for (auto &task : impl_->tasks)
      if (task.id == job->taskId && task.status != TaskStatus::Completed) {
        if (auto *person = impl_->getPerson(task.employeeId)) {
          if (person->task == task.id) {
            person->task = 0;
            person->state = PersonState::Idle;
            person->destination = person->position;
          }
        }
        task.status = TaskStatus::Completed;
        task.employeeId = 0;
        task.blockedReason = "Build cancelled";
      }
  }
  if (!job->materialsConsumed) {
    detail::subtractMaterials(impl_->construction.reservedMaterials,
                              job->reservedMaterials);
    detail::addMaterials(impl_->construction.availableMaterials,
                         job->reservedMaterials);
    impl_->economy.cashCents += job->reservedCashCents;
    impl_->economy.constructionCostCents -= job->reservedCashCents;
  }
  job->state = BuildJobState::Cancelled;
  return {true, "Build job cancelled", jobId};
}
CommandResult
Simulation::addConstructionMaterials(const ConstructionMaterials &materials) {
  if (!detail::validMaterials(materials) || materialsZero(materials))
    return {false, "Construction material quantities are invalid"};
  ConstructionMaterials combined = impl_->construction.availableMaterials;
  detail::addMaterials(combined, materials);
  if (!detail::validMaterials(combined))
    return {false, "Construction material capacity exceeded"};
  impl_->construction.availableMaterials = combined;
  return {true, "Construction materials stocked"};
}
ConstructionSnapshot Simulation::constructionSnapshot() const {
  return impl_->construction;
}

CommandResult Simulation::setRoomUtility(EntityId roomId, UtilityKind kind,
                                         bool connected) {
  if (!impl_->getRoom(roomId) ||
      (kind != UtilityKind::Power && kind != UtilityKind::Water))
    return {false, "Room or utility is invalid"};
  impl_->setRoomUtilityInternal(roomId, kind, connected);
  return {true, connected ? "Room utility connected" : "Room utility disconnected",
          roomId};
}
CommandResult Simulation::setRoomInfrastructure(EntityId roomId,
                                                InfrastructureKind kind,
                                                bool installed) {
  if (!impl_->getRoom(roomId))
    return {false, "Room not found"};
  auto &system = impl_->ensureRoomSystem(roomId);
  switch (kind) {
  case InfrastructureKind::Egress:
    system.egress = installed;
    break;
  case InfrastructureKind::Accessibility:
    system.accessible = installed;
    break;
  case InfrastructureKind::Fire:
    system.fireCovered = installed;
    break;
  case InfrastructureKind::Security:
    system.securityCovered = installed;
    break;
  }
  return {true, installed ? "Infrastructure installed" : "Infrastructure removed",
          roomId};
}
RoomSaleValidation Simulation::validateRoomForSale(EntityId roomId) const {
  if (!impl_->getRoom(roomId))
    return {false, {BuildingSystemReason::RoomNotFound}};
  return detail::validateRoomSystems(impl_->buildingSystems, roomId);
}
CommandResult Simulation::installElevator(const ElevatorSpec &spec) {
  if ((spec.kind != ElevatorKind::Passenger &&
       spec.kind != ElevatorKind::Service) ||
      spec.minFloor < 0 || spec.maxFloor >= impl_->floors ||
      spec.minFloor > spec.maxFloor || spec.startFloor < spec.minFloor ||
      spec.startFloor > spec.maxFloor || spec.capacity < 1 ||
      spec.capacity > 100 || spec.travelSecondsPerFloor < 1 ||
      spec.travelSecondsPerFloor > 600 || spec.doorSeconds < 1 ||
      spec.doorSeconds > 120)
    return {false, "Elevator specification is invalid"};
  ElevatorSnapshot elevator;
  elevator.id = impl_->nextId++;
  elevator.kind = spec.kind;
  elevator.minFloor = spec.minFloor;
  elevator.maxFloor = spec.maxFloor;
  elevator.currentFloor = spec.startFloor;
  elevator.targetFloor = spec.startFloor;
  elevator.capacity = spec.capacity;
  elevator.travelSecondsPerFloor = spec.travelSecondsPerFloor;
  elevator.doorSeconds = spec.doorSeconds;
  impl_->buildingSystems.elevators.push_back(elevator);
  return {true, "Elevator installed", elevator.id};
}
CommandResult Simulation::requestElevator(EntityId elevatorId, int pickupFloor,
                                          int destinationFloor) {
  auto *elevator = impl_->getElevator(elevatorId);
  if (!elevator || pickupFloor < elevator->minFloor ||
      pickupFloor > elevator->maxFloor ||
      destinationFloor < elevator->minFloor ||
      destinationFloor > elevator->maxFloor)
    return {false, "Elevator request is outside its served floors"};
  ElevatorRequestSnapshot request;
  request.id = impl_->nextId++;
  request.pickupFloor = pickupFloor;
  request.destinationFloor = destinationFloor;
  elevator->requests.push_back(request);
  return {true, "Elevator requested", request.id};
}
BuildingSystemsSnapshot Simulation::buildingSystemsSnapshot() const {
  return impl_->buildingSystems;
}

void Simulation::step(double seconds) {
  if (!std::isfinite(seconds) || seconds < 0 || seconds > 1.0e9)
    throw std::invalid_argument("step seconds must be finite and bounded");
  impl_->remainderMillis += (std::int64_t)std::llround(seconds * 1000);
  while (impl_->remainderMillis >= 1000) {
    impl_->remainderMillis -= 1000;
    const auto revenueBefore = impl_->final05RevenueCents();
    impl_->minute();
    impl_->tickServicesWithPhysicalLabor();
    impl_->reconcileRoomServiceCompletion();
    impl_->food.tickSecond();
    impl_->events.tickSecond();
    impl_->amenities.tickSecond();
    for (const auto &handoff : impl_->food.pendingRoomServiceHandoffs())
      if (impl_->services.markRoomServiceProductionReady(
              handoff.roomServiceOrderId))
        (void)impl_->food.acknowledgeRoomServiceHandoff(
            handoff.foodOrderId);
    const auto revenueAfter = impl_->final05RevenueCents();
    if (revenueAfter < revenueBefore)
      throw std::logic_error("FINAL-05 revenue moved backwards");
    const auto revenueDelta = revenueAfter - revenueBefore;
    impl_->economy.revenueCents += revenueDelta;
    impl_->economy.cashCents += revenueDelta;
  }
}

LogisticsSnapshot Simulation::logisticsSnapshot() const {
  return impl_->services.logisticsSnapshot();
}
HousekeepingSnapshot Simulation::housekeepingSnapshot() const {
  return impl_->services.housekeepingSnapshot();
}
EngineeringSnapshot Simulation::engineeringSnapshot() const {
  return impl_->services.engineeringSnapshot();
}
TaskId Simulation::requestRoomTurn(RoomId roomId) {
  if (!impl_->getRoom(roomId))
    return 0;
  return impl_->services.requestRoomTurn(roomId);
}
LaundryBatchId Simulation::requestLaundryBatch(int quantity) {
  return impl_->services.requestLaundryBatch(quantity);
}
WorkOrderId Simulation::createWorkOrder(AssetId assetId, WorkOrderType type) {
  return impl_->services.createWorkOrder(assetId, type);
}
RoomServiceOrderId Simulation::placeRoomServiceOrder(
    GuestId guestId, const RoomServiceOrder &order) {
  return impl_->services.placeRoomServiceOrder(guestId, order);
}
bool Simulation::markRoomServiceProductionReady(RoomServiceOrderId orderId) {
  return impl_->services.markRoomServiceProductionReady(orderId);
}
bool Simulation::requestRoomServiceTrayPickup(RoomServiceOrderId orderId) {
  return impl_->services.requestRoomServiceTrayPickup(orderId);
}
FoodOrderId Simulation::createFoodOrder(GuestId guestId,
                                        const MenuOrder &order) {
  RoomServiceOrderId deliveryId{};
  if (order.channel == FoodOrderChannel::RoomService) {
    RoomServiceOrder delivery;
    delivery.itemCount = std::max(1, order.quantity);
    deliveryId = impl_->services.placeRoomServiceOrder(guestId, delivery);
    if (deliveryId == 0)
      return 0;
  }
  const auto id = impl_->food.createFoodOrder(guestId, order, deliveryId);
  if (id != 0 && (order.channel == FoodOrderChannel::RoomService ||
                  order.channel == FoodOrderChannel::Banquet))
    (void)impl_->food.beginProduction(id);
  return id;
}
EventQuote Simulation::quoteEvent(const EventRequest &request) const {
  return impl_->events.quoteEvent(request);
}
EventBookingId Simulation::confirmEvent(const EventRequest &request) {
  return impl_->events.confirmEvent(request);
}
AmenityReservationResult Simulation::reserveAmenity(
    GuestId guestId, const AmenityRequest &request) {
  return impl_->amenities.reserveAmenity(guestId, request);
}
FoodServiceSnapshot Simulation::foodServiceSnapshot() const {
  return impl_->food.snapshot();
}
EventsSnapshot Simulation::eventsSnapshot() const {
  return impl_->events.snapshot();
}
AmenitiesSnapshot Simulation::amenitiesSnapshot() const {
  return impl_->amenities.snapshot();
}

bool Simulation::isReachable(Position a, Position b) const {
  return impl_->inside(a) && impl_->inside(b) && impl_->passable(a) &&
         impl_->passable(b) && (same(a, b) || !impl_->path(a, b).empty());
}
SimulationView Simulation::view() const {
  SimulationView v;
  v.elapsedSeconds = impl_->elapsed;
  v.day = impl_->elapsed / 86400;
  v.hour = (impl_->elapsed / 3600) % 24;
  v.width = impl_->width;
  v.height = impl_->height;
  v.floors = impl_->floors;
  for (int i = 0; i < (int)impl_->map.size(); ++i)
    if (impl_->map[i] != TileKind::Empty)
      v.tiles.push_back({{i / (impl_->width * impl_->height), i % impl_->width,
                          (i / impl_->width) % impl_->height},
                         impl_->map[i]});
  for (auto &r : impl_->rooms)
    v.rooms.push_back(r);
  for (auto &p : impl_->people)
    v.people.push_back(p);
  for (auto &r : impl_->reservations)
    v.reservations.push_back(r);
  for (auto &r : impl_->completedReservationHistory)
    v.reservations.push_back(r);
  for (auto &t : impl_->tasks)
    v.tasks.push_back(t);
  for (auto &t : impl_->completedTaskHistory)
    v.tasks.push_back(t);
  v.reviews = impl_->reviews;
  // Public inventory is the canonical FINAL-04 usable stock. The legacy
  // inventory remains private scheduler/save compatibility state until the
  // physical pickup path is fully migrated.
  const auto &logistics = impl_->services.logistics();
  v.inventory = {
      logistics.inventoryUsable("clean_linen_set"),
      logistics.inventoryUsable("towel_unit"),
      logistics.inventoryUsable("amenity_kit"),
      logistics.inventoryUsable("cleaning_chemical"),
      logistics.inventoryUsable("maintenance_part")};
  v.economy = impl_->economy;
  for (auto &o : impl_->orders)
    v.supplyOrders.push_back(o);
  v.departments = departments();
  return v;
}

std::string Simulation::save() const {
  std::ostringstream o;
  o << std::setprecision(17) << "HHGS 9 " << impl_->seed << ' ' << impl_->width
    << ' ' << impl_->height << ' ' << impl_->floors << ' ' << impl_->elapsed
    << ' ' << impl_->remainderMillis << ' ' << impl_->nextId << ' '
    << impl_->baseDemand << ' ' << impl_->utilityPerRoomDayCents << ' '
    << impl_->turnoverWork << ' ' << impl_->repairWork << ' '
    << impl_->checkInWork << ' ' << impl_->hungerRate << ' ' << impl_->restLoss
    << ' ' << impl_->roomConditionLossPerDay << '\n'
    << impl_->rng << '\n';
  o << impl_->map.size();
  for (auto x : impl_->map)
    o << ' ' << ei(x);
  o << '\n';
  auto inv = [&](const InventoryView &x) {
    o << x.linen << ' ' << x.towels << ' ' << x.amenities << ' ' << x.chemicals
      << ' ' << x.parts;
  };
  inv(impl_->inventory);
  o << '\n';
  auto &e = impl_->economy;
  o << e.cashCents << ' ' << e.revenueCents << ' ' << e.payrollCents << ' '
    << e.supplyCostCents << ' ' << e.constructionCostCents << ' '
    << e.utilityCostCents << ' ' << e.occupancy << ' ' << e.reputation << ' '
    << e.completedStays << ' ' << e.stars << ' ' << e.distressed << '\n';
  o << impl_->rooms.size() << '\n';
  for (auto &r : impl_->rooms)
    o << r.id << ' ' << std::quoted(r.name) << ' ' << r.door.floor << ' '
      << r.door.x << ' ' << r.door.y << ' ' << ei(r.status) << ' ' << r.floor
      << ' ' << r.x << ' ' << r.y << ' ' << r.width << ' ' << r.height << ' '
      << r.beds << ' ' << r.baths << ' ' << r.cleanliness << ' ' << r.condition
      << ' ' << r.nightlyRateCents << ' ' << r.reservationId << ' '
      << r.reachable << ' ' << r.closed << '\n';
  o << impl_->people.size() << '\n';
  for (auto &p : impl_->people)
    o << p.id << ' ' << std::quoted(p.name) << ' ' << ei(p.kind) << ' '
      << ei(p.state) << ' ' << p.position.floor << ' ' << p.position.x << ' '
      << p.position.y << ' ' << p.destination.floor << ' ' << p.destination.x
      << ' ' << p.destination.y << ' ' << p.fatigue << ' ' << p.satisfaction
      << ' ' << p.hunger << ' ' << p.rest << ' ' << p.patience << ' ' << p.skill
      << ' ' << p.shiftStartHour << ' ' << p.shiftEndHour << ' '
      << p.travelSeconds << ' ' << p.queueWaitSeconds << ' '
      << std::quoted(p.goal) << ' ' << p.onShift << ' ' << p.hourlyWageCents
      << ' ' << p.reservation << ' ' << p.task << ' ' << p.accruedWageUnits
      << '\n';
  o << impl_->reservations.size() + impl_->completedReservationHistory.size()
    << '\n';
  for (auto &r : impl_->reservations)
    o << r.id << ' ' << std::quoted(r.guestName) << ' ' << r.roomId << ' '
      << r.arrivalDay << ' ' << r.departureDay << ' ' << r.nightlyRateCents
      << ' ' << r.checkedIn << ' ' << r.completed << ' ' << r.satisfaction
      << ' ' << r.arrived << ' ' << r.checkoutStarted << ' '
      << r.checkoutCleanliness << '\n';
  for (auto &r : impl_->completedReservationHistory)
    o << r.id << ' ' << std::quoted(r.guestName) << ' ' << r.roomId << ' '
      << r.arrivalDay << ' ' << r.departureDay << ' ' << r.nightlyRateCents
      << ' ' << r.checkedIn << ' ' << r.completed << ' ' << r.satisfaction
      << ' ' << r.arrived << ' ' << r.checkoutStarted << ' '
      << r.checkoutCleanliness << '\n';
  o << impl_->tasks.size() + impl_->completedTaskHistory.size() << '\n';
  for (auto &t : impl_->tasks)
    o << t.id << ' ' << ei(t.kind) << ' ' << ei(t.status) << ' ' << t.targetId
      << ' ' << t.employeeId << ' ' << t.target.floor << ' ' << t.target.x
      << ' ' << t.target.y << ' ' << t.workRemainingSeconds << ' '
      << std::quoted(t.blockedReason) << ' ' << t.total << ' '
      << t.resourcesClaimed << '\n';
  for (auto &t : impl_->completedTaskHistory)
    o << t.id << ' ' << ei(t.kind) << ' ' << ei(t.status) << ' ' << t.targetId
      << ' ' << t.employeeId << ' ' << t.target.floor << ' ' << t.target.x
      << ' ' << t.target.y << ' ' << t.workRemainingSeconds << ' '
      << std::quoted(t.blockedReason) << ' ' << t.total << ' '
      << t.resourcesClaimed << '\n';
  o << impl_->reviews.size() << '\n';
  for (auto &r : impl_->reviews)
    o << r.reservationId << ' ' << r.day << ' ' << r.score << ' '
      << std::quoted(r.text) << '\n';
  o << impl_->orders.size() << '\n';
  for (auto &p : impl_->orders) {
    o << p.id << ' ';
    inv(p.items);
    o << ' ' << p.etaDay << ' ' << p.delivered << '\n';
  }
  const auto serviceState = impl_->services.save();
  o << "FINAL04 " << serviceState.size() << '\n';
  o.write(serviceState.data(), static_cast<std::streamsize>(serviceState.size()));
  o << '\n';
  const auto foodState = impl_->food.save();
  o << "FINAL05_FOOD " << foodState.size() << '\n';
  o.write(foodState.data(), static_cast<std::streamsize>(foodState.size()));
  o << '\n';
  const auto eventState = impl_->events.save();
  o << "FINAL05_EVENTS " << eventState.size() << '\n';
  o.write(eventState.data(), static_cast<std::streamsize>(eventState.size()));
  o << '\n';
  const auto amenityState = impl_->amenities.save();
  o << "FINAL05_AMENITIES " << amenityState.size() << '\n';
  o.write(amenityState.data(), static_cast<std::streamsize>(amenityState.size()));
  o << '\n';
  return o.str();
}
Simulation Simulation::load(std::string_view data) {
  if (data.size() > 64 * 1024 * 1024)
    throw std::invalid_argument("simulation save too large");
  std::istringstream i{std::string(data)};
  std::string magic;
  int version, w, h, f;
  i >> magic >> version;
  if (magic != "HHGS" || version < 2 || version > 9)
    throw std::invalid_argument("unsupported simulation save");
  std::uint64_t seed;
  i >> seed >> w >> h >> f;
  if (w < 4 || w > MaxMapWidth || h < 4 || h > MaxMapHeight ||
      f < 1 || f > MaxMapFloors)
    throw std::invalid_argument("invalid saved map dimensions");
  Simulation s(seed, w, h, f);
  auto &d = *s.impl_;
  i >> d.elapsed >> d.remainderMillis >> d.nextId >> d.baseDemand >>
      d.utilityPerRoomDayCents >> d.turnoverWork >> d.repairWork >>
      d.checkInWork >> d.hungerRate >> d.restLoss;
  if (version >= 5)
    i >> d.roomConditionLossPerDay;
  i >> d.rng;
  if (!i || d.elapsed < 0 || d.remainderMillis < 0 ||
      d.remainderMillis >= 1000 || d.nextId == 0 ||
      !std::isfinite(d.baseDemand) || d.baseDemand < 0 ||
      d.utilityPerRoomDayCents < 0 || !std::isfinite(d.turnoverWork) ||
      d.turnoverWork <= 0 || !std::isfinite(d.repairWork) ||
      d.repairWork <= 0 || !std::isfinite(d.checkInWork) ||
      d.checkInWork <= 0 || !std::isfinite(d.hungerRate) || d.hungerRate < 0 ||
      !std::isfinite(d.restLoss) || d.restLoss < 0 ||
      !std::isfinite(d.roomConditionLossPerDay) ||
      d.roomConditionLossPerDay < 0 || d.roomConditionLossPerDay > 100)
    throw std::invalid_argument("invalid saved simulation settings");
  size_t n;
  i >> n;
  if (n != (size_t)w * h * f)
    throw std::invalid_argument("invalid saved tile count");
  d.map.resize(n);
  for (auto &x : d.map) {
    int q;
    i >> q;
    if (q < ei(TileKind::Empty) || q > ei(TileKind::Lobby))
      throw std::invalid_argument("invalid saved tile");
    x = (TileKind)q;
  }
  auto inv = [&](InventoryView &x) {
    i >> x.linen >> x.towels >> x.amenities >> x.chemicals >> x.parts;
  };
  inv(d.inventory);
  if (d.inventory.linen < 0 || d.inventory.towels < 0 ||
      d.inventory.amenities < 0 || d.inventory.chemicals < 0 ||
      d.inventory.parts < 0)
    throw std::invalid_argument("invalid saved inventory");
  auto &e = d.economy;
  i >> e.cashCents >> e.revenueCents >> e.payrollCents >> e.supplyCostCents >>
      e.constructionCostCents >> e.utilityCostCents >> e.occupancy >>
      e.reputation >> e.completedStays >> e.stars >> e.distressed;
  if (!i || !std::isfinite(e.occupancy) || e.occupancy < 0 || e.occupancy > 1 ||
      !std::isfinite(e.reputation) || e.reputation < 0 || e.reputation > 100 ||
      e.completedStays < 0 || e.stars < 1 || e.stars > 5)
    throw std::invalid_argument("invalid saved economy");
  i >> n;
  if (n > 100000)
    throw std::invalid_argument("too many saved rooms");
  d.rooms.resize(n);
  for (auto &r : d.rooms) {
    int st;
    double legacyNightlyRate{};
    i >> r.id >> std::quoted(r.name) >> r.door.floor >> r.door.x >> r.door.y >>
        st >> r.floor >> r.x >> r.y >> r.width >> r.height >> r.beds >>
        r.baths >> r.cleanliness >> r.condition;
    if (version >= 6)
      i >> r.nightlyRateCents;
    else
      i >> legacyNightlyRate;
    i >> r.reservationId >> r.reachable >> r.closed;
    if (version < 6) {
      if (!std::isfinite(legacyNightlyRate) || legacyNightlyRate <= 0 ||
          legacyNightlyRate > 5000)
        throw std::invalid_argument("invalid saved room rate");
      r.nightlyRateCents =
          static_cast<std::int64_t>(std::llround(legacyNightlyRate * 100.0));
    }
    r.nightlyRate = r.nightlyRateCents / 100.0;
    if (st < ei(RoomStatus::Incomplete) || st > ei(RoomStatus::OutOfOrder) ||
        r.width < 3 || r.height < 3 || !d.inside(r.door) ||
        !d.roomFootprintBounds(r.floor, r.x, r.y, r.width, r.height) ||
        !std::isfinite(r.cleanliness) || !std::isfinite(r.condition) ||
        r.nightlyRateCents <= 0 || r.nightlyRateCents > 500000)
      throw std::invalid_argument("invalid saved room");
    r.status = static_cast<RoomStatus>(st);
  }
  i >> n;
  if (n > 100000)
    throw std::invalid_argument("too many saved people");
  d.people.resize(n);
  for (auto &p : d.people) {
    int k, st;
    double legacyWage{};
    std::int64_t legacyAccruedWageMicros{};
    i >> p.id >> std::quoted(p.name) >> k >> st >> p.position.floor >>
        p.position.x >> p.position.y >> p.destination.floor >>
        p.destination.x >> p.destination.y >> p.fatigue >> p.satisfaction >>
        p.hunger >> p.rest >> p.patience >> p.skill >> p.shiftStartHour >>
        p.shiftEndHour;
    if (version >= 3)
      i >> p.travelSeconds;
    i >> p.queueWaitSeconds >> std::quoted(p.goal) >> p.onShift;
    if (version >= 6)
      i >> p.hourlyWageCents;
    else
      i >> legacyWage;
    i >> p.reservation >> p.task;
    if (version >= 6)
      i >> p.accruedWageUnits;
    else
      i >> legacyAccruedWageMicros;
    if (version < 6) {
      if (!std::isfinite(legacyWage) || legacyWage < 0 || legacyWage > 10000 ||
          legacyAccruedWageMicros < 0 ||
          legacyAccruedWageMicros > 1000000000000000LL)
        throw std::invalid_argument("invalid legacy saved wage");
      p.hourlyWageCents =
          static_cast<std::int64_t>(std::llround(legacyWage * 100.0));
      p.accruedWageUnits = (legacyAccruedWageMicros * 36 + 50) / 100;
    }
    if (k < ei(PersonKind::Guest) || k > ei(PersonKind::Maintenance) ||
        st < ei(PersonState::OffDuty) || st > ei(PersonState::CheckedOut) ||
        !d.inside(p.position) || !d.inside(p.destination) ||
        !std::isfinite(p.fatigue) || p.fatigue < 0 || p.fatigue > 100 ||
        !std::isfinite(p.satisfaction) || p.satisfaction < 0 ||
        p.satisfaction > 100 || !std::isfinite(p.hunger) || p.hunger < 0 ||
        p.hunger > 100 || !std::isfinite(p.rest) || p.rest < 0 ||
        p.rest > 100 || !std::isfinite(p.patience) || p.patience < 0 ||
        p.patience > 100 || !std::isfinite(p.skill) || p.skill < 0 ||
        p.skill > 100 || p.shiftStartHour < 0 || p.shiftStartHour > 23 ||
        p.shiftEndHour < 0 || p.shiftEndHour > 23 || p.travelSeconds < 0 ||
        p.queueWaitSeconds < 0 || p.hourlyWageCents < 0 ||
        p.hourlyWageCents > 1000000 || p.accruedWageUnits < 0)
      throw std::invalid_argument("invalid saved person");
    p.kind = static_cast<PersonKind>(k);
    p.state = static_cast<PersonState>(st);
  }
  i >> n;
  if (n > 100000)
    throw std::invalid_argument("too many saved reservations");
  d.reservations.resize(n);
  for (auto &r : d.reservations) {
    double legacyNightlyRate{};
    i >> r.id >> std::quoted(r.guestName) >> r.roomId >> r.arrivalDay >>
        r.departureDay;
    if (version >= 6)
      i >> r.nightlyRateCents;
    else
      i >> legacyNightlyRate;
    i >> r.checkedIn >> r.completed >> r.satisfaction >> r.arrived;
    if (version >= 4)
      i >> r.checkoutStarted >> r.checkoutCleanliness;
    else if (r.completed)
      r.checkoutStarted = true;
    if (version < 6) {
      if (!std::isfinite(legacyNightlyRate) || legacyNightlyRate <= 0 ||
          legacyNightlyRate > 5000)
        throw std::invalid_argument("invalid saved reservation rate");
      r.nightlyRateCents =
          static_cast<std::int64_t>(std::llround(legacyNightlyRate * 100.0));
    }
    r.nightlyRate = r.nightlyRateCents / 100.0;
  }
  for (const auto &r : d.reservations)
    if (r.arrivalDay < 0 || r.departureDay <= r.arrivalDay ||
        r.nightlyRateCents <= 0 || r.nightlyRateCents > 500000 ||
        !std::isfinite(r.satisfaction) || r.satisfaction < 0 ||
        r.satisfaction > 100 || !std::isfinite(r.checkoutCleanliness) ||
        r.checkoutCleanliness < 0 || r.checkoutCleanliness > 100)
      throw std::invalid_argument("invalid saved reservation");
  i >> n;
  if (n > 100000)
    throw std::invalid_argument("too many saved tasks");
  d.tasks.resize(n);
  for (auto &t : d.tasks) {
    int k, st;
    i >> t.id >> k >> st >> t.targetId >> t.employeeId >> t.target.floor >>
        t.target.x >> t.target.y >> t.workRemainingSeconds >>
        std::quoted(t.blockedReason) >> t.total >> t.resourcesClaimed;
    const bool completed = st == ei(TaskStatus::Completed);
    if (k < ei(TaskKind::CheckIn) || k > ei(TaskKind::CheckOut) ||
        st < ei(TaskStatus::Ready) || st > ei(TaskStatus::Completed) ||
        !d.inside(t.target) || !std::isfinite(t.workRemainingSeconds) ||
        !std::isfinite(t.total) || t.total <= 0 ||
        (completed ? t.workRemainingSeconds > 0
                   : (t.workRemainingSeconds <= 0 ||
                      t.workRemainingSeconds > t.total)))
      throw std::invalid_argument("invalid saved task");
    t.kind = static_cast<TaskKind>(k);
    t.status = static_cast<TaskStatus>(st);
  }
  i >> n;
  if (n > 100000)
    throw std::invalid_argument("too many saved reviews");
  d.reviews.resize(n);
  for (auto &r : d.reviews)
    i >> r.reservationId >> r.day >> r.score >> std::quoted(r.text);
  for (const auto &r : d.reviews)
    if (r.day < 0 || r.score < 0 || r.score > 100)
      throw std::invalid_argument("invalid saved review");
  i >> n;
  if (n > 100000)
    throw std::invalid_argument("too many saved orders");
  d.orders.resize(n);
  for (auto &p : d.orders) {
    i >> p.id;
    inv(p.items);
    i >> p.etaDay >> p.delivered;
    if (p.items.linen < 0 || p.items.towels < 0 || p.items.amenities < 0 ||
        p.items.chemicals < 0 || p.items.parts < 0 || p.etaDay < 0)
      throw std::invalid_argument("invalid saved order");
  }
  if (version >= 8) {
    std::string final04Tag;
    std::size_t serviceBytes{};
    i >> final04Tag >> serviceBytes;
    if (!i || final04Tag != "FINAL04" || serviceBytes > 16 * 1024 * 1024)
      throw std::invalid_argument("invalid FINAL-04 save section");
    if (i.get() != '\n')
      throw std::invalid_argument("invalid FINAL-04 save delimiter");
    std::string serviceState(serviceBytes, '\0');
    i.read(serviceState.data(), static_cast<std::streamsize>(serviceBytes));
    if (!i || static_cast<std::size_t>(i.gcount()) != serviceBytes)
      throw std::invalid_argument("truncated FINAL-04 save section");
    d.services = ServiceLogisticsRuntime::load(serviceState);
    if (i.get() != '\n')
      throw std::invalid_argument("invalid FINAL-04 save terminator");
  } else {
    for (const auto &room : d.rooms) {
      d.services.registerRoom(
          room.id, room.status == RoomStatus::VacantReady ? ServiceRoomStatus::Ready
                                                          : ServiceRoomStatus::Blocked);
      d.services.registerAsset(
          room.id,
          std::clamp(static_cast<int>(std::llround(room.condition * 100.0)), 0, 10000));
    }
    d.services.synchronizeElapsedSecondsForSimulation(d.elapsed);
  }
  if (d.services.elapsedSeconds() != d.elapsed ||
      d.services.registeredRoomCount() != d.rooms.size() ||
      d.services.registeredAssetCount() != d.rooms.size())
    throw std::invalid_argument("FINAL-04 state does not match simulation");
  for (const auto &room : d.rooms)
    if (!d.services.hasRegisteredRoom(room.id) ||
        !d.services.hasRegisteredAsset(room.id))
      throw std::invalid_argument("FINAL-04 room registrations do not match simulation");

  if (version >= 9) {
    auto readFinal05Section = [&](std::string_view expected) {
      std::string tag;
      std::size_t bytes{};
      i >> tag >> bytes;
      if (!i || tag != expected || bytes > 16 * 1024 * 1024)
        throw std::invalid_argument("invalid FINAL-05 save section");
      if (i.get() != '\n')
        throw std::invalid_argument("invalid FINAL-05 save delimiter");
      std::string state(bytes, '\0');
      i.read(state.data(), static_cast<std::streamsize>(bytes));
      if (!i || static_cast<std::size_t>(i.gcount()) != bytes)
        throw std::invalid_argument("truncated FINAL-05 save section");
      if (i.get() != '\n')
        throw std::invalid_argument("invalid FINAL-05 save terminator");
      return state;
    };
    d.food = FoodServiceSystem::load(readFinal05Section("FINAL05_FOOD"));
    d.events = EventsSystem::load(readFinal05Section("FINAL05_EVENTS"));
    d.amenities = AmenitiesSystem::load(
        readFinal05Section("FINAL05_AMENITIES"));
    if (d.food.snapshot().elapsedSeconds != d.elapsed ||
        d.events.snapshot().elapsedSeconds != d.elapsed ||
        d.amenities.snapshot().elapsedSeconds != d.elapsed)
      throw std::invalid_argument("FINAL-05 clock does not match simulation");
  } else {
    d.food.setElapsedSeconds(d.elapsed);
    d.events.setElapsedSeconds(d.elapsed);
    d.amenities.setElapsedSeconds(d.elapsed);
  }
  if (!i)
    throw std::invalid_argument("corrupt simulation save");
  i >> std::ws;
  if (!i.eof())
    throw std::invalid_argument("unexpected trailing save data");

  std::unordered_set<EntityId> entityIds;
  auto addEntityId = [&](EntityId id) {
    if (id == 0 || id >= d.nextId || !entityIds.insert(id).second)
      throw std::invalid_argument("invalid saved entity IDs");
  };
  std::unordered_map<EntityId, const Room *> roomById;
  std::unordered_map<EntityId, const Person *> personById;
  std::unordered_map<EntityId, const Reservation *> reservationById;
  std::unordered_map<EntityId, const Task *> taskById;

  const auto entrance = d.entrance();
  const auto reachable = d.reachableMask(entrance);
  std::vector<bool> claimedRoomTiles(d.map.size(), false);
  for (const auto &room : d.rooms) {
    addEntityId(room.id);
    roomById.emplace(room.id, &room);
    if (room.floor != room.door.floor ||
        d.map[d.index(room.door)] != TileKind::Door || room.cleanliness < 0 ||
        room.cleanliness > 100 || room.condition < 0 || room.condition > 100 ||
        (room.closed && room.status != RoomStatus::OutOfOrder))
      throw std::invalid_argument("invalid saved room state");

    const auto bounds =
        d.roomFootprintBounds(room.floor, room.x, room.y, room.width, room.height);
    if (!bounds)
      throw std::invalid_argument("invalid saved room footprint");
    for (int y = bounds->top; y < bounds->bottomExclusive; ++y)
      for (int x = bounds->left; x < bounds->rightExclusive; ++x) {
        const auto tileIndex =
            static_cast<std::size_t>(d.index({room.floor, x, y}));
        if (claimedRoomTiles[tileIndex])
          throw std::invalid_argument("overlapping saved room footprints");
        if (d.map[tileIndex] == TileKind::Empty)
          throw std::invalid_argument("saved room footprint contains empty tile");
        claimedRoomTiles[tileIndex] = true;
      }

    const bool actualReachable =
        d.inside(room.door) &&
        reachable[static_cast<std::size_t>(d.index(room.door))] &&
        !same(entrance, room.door);
    if (room.reachable != actualReachable)
      throw std::invalid_argument("saved room reachability does not match map");
  }
  for (const auto &person : d.people) {
    addEntityId(person.id);
    personById.emplace(person.id, &person);
  }
  for (const auto &reservation : d.reservations) {
    addEntityId(reservation.id);
    reservationById.emplace(reservation.id, &reservation);
  }
  for (const auto &task : d.tasks) {
    addEntityId(task.id);
    taskById.emplace(task.id, &task);
  }
  for (const auto &order : d.orders)
    addEntityId(order.id);

  std::unordered_map<EntityId, int> guestsByReservation;
  for (const auto &person : d.people) {
    if (person.kind == PersonKind::Guest) {
      if (!reservationById.contains(person.reservation) || person.task != 0)
        throw std::invalid_argument("invalid saved guest references");
      ++guestsByReservation[person.reservation];
      if (person.state == PersonState::CheckedOut &&
          !reservationById.at(person.reservation)->completed)
        throw std::invalid_argument("invalid saved guest lifecycle");
    } else {
      if (person.reservation != 0)
        throw std::invalid_argument("invalid saved employee references");
      if (person.task != 0) {
        const auto task = taskById.find(person.task);
        if (task == taskById.end() || task->second->employeeId != person.id ||
            task->second->status == TaskStatus::Completed)
          throw std::invalid_argument("invalid saved employee task");
      }
    }
  }
  for (const auto &reservation : d.reservations) {
    const auto room = roomById.find(reservation.roomId);
    if ((!reservation.completed && room == roomById.end()) ||
        (reservation.checkedIn && !reservation.arrived) ||
        (reservation.checkoutStarted && !reservation.checkedIn) ||
        (reservation.completed && !reservation.checkoutStarted) ||
        (!reservation.completed &&
         guestsByReservation[reservation.id] != (reservation.arrived ? 1 : 0)) ||
        (reservation.completed && guestsByReservation[reservation.id] > 1))
      throw std::invalid_argument("invalid saved reservation references");
    if (!reservation.checkoutStarted && !reservation.completed &&
        room->second->reservationId != reservation.id)
      throw std::invalid_argument("invalid saved room assignment");
  }
  for (const auto &room : d.rooms) {
    if (room.reservationId != 0) {
      const auto reservation = reservationById.find(room.reservationId);
      if (reservation == reservationById.end() ||
          reservation->second->roomId != room.id ||
          reservation->second->checkoutStarted ||
          reservation->second->completed)
        throw std::invalid_argument("invalid saved room reservation");
    }
    if ((room.status == RoomStatus::Reserved ||
         room.status == RoomStatus::Occupied) != (room.reservationId != 0))
      throw std::invalid_argument("invalid saved room occupancy");
  }
  for (const auto &task : d.tasks) {
    const bool active = task.status != TaskStatus::Completed;
    const bool guestTask =
        task.kind == TaskKind::CheckIn || task.kind == TaskKind::CheckOut;
    if (active &&
        ((guestTask && !personById.contains(task.targetId)) ||
         ((task.kind == TaskKind::Turnover || task.kind == TaskKind::Repair) &&
          !roomById.contains(task.targetId))))
      throw std::invalid_argument("invalid saved task target");
    if (task.status == TaskStatus::Traveling ||
        task.status == TaskStatus::Working) {
      const auto employee = personById.find(task.employeeId);
      if (employee == personById.end() ||
          employee->second->kind == PersonKind::Guest ||
          employee->second->task != task.id)
        throw std::invalid_argument("invalid saved task employee");
    } else if (active && task.employeeId != 0) {
      throw std::invalid_argument("invalid saved unassigned task");
    }
  }
  for (const auto &review : d.reviews) {
    const auto reservation = reservationById.find(review.reservationId);
    if (reservation == reservationById.end() || !reservation->second->completed)
      throw std::invalid_argument("invalid saved review reference");
  }
  d.compactTransientState();
  return s;
}

} // namespace hh::game
