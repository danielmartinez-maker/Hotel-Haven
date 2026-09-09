#include "hh/game/Simulation.h"
#include "hh/game/BuildJobs.h"
#include "hh/game/BuildingSystems.h"
#include "hh/game/Construction.h"
#include "hh/assets/Json.h"
#include "StaffOptimization.h"
#include "hh/optimization/Types.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <iomanip>
#include <limits>
#include <queue>
#include <random>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace hh::game {
namespace {
bool same(Position a, Position b) {
  return a.floor == b.floor && a.x == b.x && a.y == b.y;
}
int manhattan(Position a, Position b) {
  return std::abs(a.x - b.x) + std::abs(a.y - b.y) +
         std::abs(a.floor - b.floor) * 8;
}
template <class E> int ei(E e) { return static_cast<int>(e); }
bool passableKind(TileKind kind) {
  return kind == TileKind::Floor || kind == TileKind::Door ||
         kind == TileKind::Entrance || kind == TileKind::FrontDesk ||
         kind == TileKind::SupplyCloset || kind == TileKind::Stairs ||
         kind == TileKind::Bathroom || kind == TileKind::StaffRoom ||
         kind == TileKind::Lobby;
}
struct Room : RoomView {};
struct Person : PersonView {
  EntityId task{};
  std::int64_t accruedWageUnits{};
};
struct Reservation : ReservationView {
  double checkoutCleanliness{100};
  bool arrived{};
};
bool reservationFinished(const Reservation &reservation) {
  return reservation.completed || reservation.walkedRelocated;
}
struct Task : TaskView {
  double total{};
  bool resourcesClaimed{};
};
struct PendingOrder : SupplyOrderView {};

struct GuestArchetypeDefaults {
  GuestArchetype archetype;
  int weight;
  std::int64_t budgetPerNightCents;
  double priceSensitivity;
  double serviceSensitivity;
  double cleanlinessSensitivity;
  double noiseSensitivity;
  double privacySensitivity;
  double safetySensitivity;
  double comfortSensitivity;
  double foodSensitivity;
  double patience;
};

constexpr std::array<GuestArchetypeDefaults, 13> guestArchetypeDefaults{{
    {GuestArchetype::BudgetLeisure, 13, 9000, .90, .45, .55, .40, .35, .55,
     .55, .45, .55},
    {GuestArchetype::Backpacker, 8, 7000, .95, .35, .40, .35, .25, .45, .35,
     .35, .70},
    {GuestArchetype::BusinessTraveler, 18, 18000, .45, .75, .80, .85, .55,
     .65, .70, .70, .45},
    {GuestArchetype::ExecutiveBusiness, 7, 26000, .25, .90, .90, .85, .80,
     .80, .90, .75, .40},
    {GuestArchetype::CoupleLeisure, 14, 17000, .55, .60, .70, .65, .60, .65,
     .75, .70, .65},
    {GuestArchetype::FamilyLeisure, 12, 19000, .65, .70, .85, .65, .35, .85,
     .75, .75, .55},
    {GuestArchetype::LuxuryLeisure, 5, 30000, .20, .95, .95, .85, .90, .90,
     .98, .90, .45},
    {GuestArchetype::ConferenceDelegate, 7, 16000, .50, .70, .75, .75, .50,
     .65, .65, .70, .50},
    {GuestArchetype::GroupTourTraveler, 5, 11000, .80, .45, .60, .50, .30,
     .65, .50, .55, .60},
    {GuestArchetype::AirportTransitTraveler, 4, 13000, .70, .55, .70, .65,
     .40, .70, .60, .45, .35},
    {GuestArchetype::WellnessTraveler, 3, 22000, .35, .80, .85, .75, .80,
     .75, .85, .75, .75},
    {GuestArchetype::VipCelebrity, 2, 30000, .10, .98, .98, .95, .98, .95,
     1.0, .90, .25},
    {GuestArchetype::CriticReviewer, 2, 22000, .35, 1.0, 1.0, .90, .85, .85,
     .95, .95, .35},
}};
constexpr double normalReviewReputationWeight = .15;
constexpr double criticReviewReputationWeight =
    normalReviewReputationWeight * 5.0;
static_assert(criticReviewReputationWeight == .75);

std::uint64_t mixedSeed(std::uint64_t seed, std::uint64_t a,
                        std::uint64_t b, std::uint64_t c,
                        std::uint64_t stream = 0) {
  auto mix = [](std::uint64_t value) {
    value += 0x9e3779b97f4a7c15ULL;
    value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31);
  };
  return mix(seed ^ mix(a) ^ mix(b + 0x632be59bd9b4e019ULL) ^
             mix(c + 0x8cb92baa7f3d8dd7ULL) ^ mix(stream));
}

double randomUnit(std::mt19937_64 &random) {
  return static_cast<double>(random() >> 11) *
         (1.0 / static_cast<double>(std::uint64_t{1} << 53));
}

bool traitConflicts(std::uint32_t flags, GuestTrait candidate) {
  const auto has = [&](GuestTrait trait) {
    return (flags & guestTraitFlag(trait)) != 0;
  };
  switch (candidate) {
  case GuestTrait::Patient:
    return has(GuestTrait::Impatient);
  case GuestTrait::Impatient:
    return has(GuestTrait::Patient);
  case GuestTrait::Neat:
    return has(GuestTrait::Messy);
  case GuestTrait::Messy:
    return has(GuestTrait::Neat);
  case GuestTrait::LightSleeper:
    return has(GuestTrait::HeavySleeper);
  case GuestTrait::HeavySleeper:
    return has(GuestTrait::LightSleeper);
  case GuestTrait::Social:
    return has(GuestTrait::Private);
  case GuestTrait::Private:
    return has(GuestTrait::Social);
  case GuestTrait::EarlyRiser:
    return has(GuestTrait::NightOwl);
  case GuestTrait::NightOwl:
    return has(GuestTrait::EarlyRiser);
  default:
    return false;
  }
}

GuestProfileView generateGuestProfile(std::mt19937_64 &random) {
  int roll = static_cast<int>(random() % 100);
  const GuestArchetypeDefaults *defaults = &guestArchetypeDefaults.back();
  for (const auto &candidate : guestArchetypeDefaults) {
    if (roll < candidate.weight) {
      defaults = &candidate;
      break;
    }
    roll -= candidate.weight;
  }
  const auto varied = [&](double mean) {
    return std::clamp(mean + (randomUnit(random) * 2.0 - 1.0) * .08, 0.0,
                      1.0);
  };
  GuestProfileView profile;
  profile.archetype = defaults->archetype;
  profile.budgetPerNightCents = static_cast<std::int64_t>(std::llround(
      defaults->budgetPerNightCents * (.90 + randomUnit(random) * .20)));
  profile.priceSensitivity = varied(defaults->priceSensitivity);
  profile.serviceSensitivity = varied(defaults->serviceSensitivity);
  profile.cleanlinessSensitivity = varied(defaults->cleanlinessSensitivity);
  profile.noiseSensitivity = varied(defaults->noiseSensitivity);
  profile.privacySensitivity = varied(defaults->privacySensitivity);
  profile.safetySensitivity = varied(defaults->safetySensitivity);
  profile.comfortSensitivity = varied(defaults->comfortSensitivity);
  profile.foodSensitivity = varied(defaults->foodSensitivity);
  profile.patience = varied(defaults->patience);

  const int traitCount = static_cast<int>(random() % 4);
  for (int attempts = 0;
       std::popcount(profile.traitFlags) < traitCount && attempts < 64;
       ++attempts) {
    const auto trait = static_cast<GuestTrait>(random() % 17);
    const auto flag = guestTraitFlag(trait);
    if ((profile.traitFlags & flag) == 0 &&
        !traitConflicts(profile.traitFlags, trait))
      profile.traitFlags |= flag;
  }
  const auto has = [&](GuestTrait trait) {
    return (profile.traitFlags & guestTraitFlag(trait)) != 0;
  };
  if (has(GuestTrait::Neat))
    profile.cleanlinessSensitivity =
        std::min(1.0, profile.cleanlinessSensitivity + .12);
  if (has(GuestTrait::Messy))
    profile.cleanlinessSensitivity =
        std::max(0.0, profile.cleanlinessSensitivity - .10);
  if (has(GuestTrait::LightSleeper))
    profile.noiseSensitivity = std::min(1.0, profile.noiseSensitivity + .15);
  if (has(GuestTrait::HeavySleeper))
    profile.noiseSensitivity = std::max(0.0, profile.noiseSensitivity - .15);
  if (has(GuestTrait::Foodie))
    profile.foodSensitivity = std::min(1.0, profile.foodSensitivity + .20);
  if (has(GuestTrait::Workaholic))
    profile.serviceSensitivity = std::min(1.0, profile.serviceSensitivity + .10);
  if (has(GuestTrait::Social))
    profile.privacySensitivity = std::max(0.0, profile.privacySensitivity - .15);
  if (has(GuestTrait::Private))
    profile.privacySensitivity = std::min(1.0, profile.privacySensitivity + .20);
  if (has(GuestTrait::Frugal)) {
    profile.priceSensitivity = std::min(1.0, profile.priceSensitivity + .20);
    profile.budgetPerNightCents = profile.budgetPerNightCents * 9 / 10;
  }
  if (has(GuestTrait::StatusConscious)) {
    profile.serviceSensitivity = std::min(1.0, profile.serviceSensitivity + .15);
    profile.comfortSensitivity = std::min(1.0, profile.comfortSensitivity + .15);
  }
  if (has(GuestTrait::FitnessFocused))
    profile.comfortSensitivity = std::min(1.0, profile.comfortSensitivity + .08);
  if (has(GuestTrait::EarlyRiser))
    profile.patience = std::min(1.0, profile.patience + .05);
  if (has(GuestTrait::NightOwl))
    profile.patience = std::max(0.0, profile.patience - .03);
  if (has(GuestTrait::ComplaintProne))
    profile.serviceSensitivity = std::min(1.0, profile.serviceSensitivity + .18);
  if (has(GuestTrait::Forgiving))
    profile.serviceSensitivity = std::max(0.0, profile.serviceSensitivity - .10);
  return profile;
}

bool validGuestProfile(const GuestProfileView &profile) {
  const auto archetype = static_cast<int>(profile.archetype);
  const auto normalized = [](double value) {
    return std::isfinite(value) && value >= 0 && value <= 1;
  };
  constexpr auto validTraitFlags =
      (guestTraitFlag(GuestTrait::Forgiving) << 1) - 1;
  if (archetype < 0 || archetype >= 13 || profile.budgetPerNightCents < 4000 ||
      profile.budgetPerNightCents > 1'000'000 ||
      !normalized(profile.priceSensitivity) ||
      !normalized(profile.serviceSensitivity) ||
      !normalized(profile.cleanlinessSensitivity) ||
      !normalized(profile.noiseSensitivity) ||
      !normalized(profile.privacySensitivity) ||
      !normalized(profile.safetySensitivity) ||
      !normalized(profile.comfortSensitivity) ||
      !normalized(profile.foodSensitivity) || !normalized(profile.patience) ||
      (profile.traitFlags & ~validTraitFlags) != 0 ||
      std::popcount(profile.traitFlags) > 3)
    return false;
  for (GuestTrait trait : {GuestTrait::Patient, GuestTrait::Impatient,
                           GuestTrait::Neat, GuestTrait::Messy,
                           GuestTrait::LightSleeper, GuestTrait::HeavySleeper,
                           GuestTrait::Social, GuestTrait::Private,
                           GuestTrait::EarlyRiser, GuestTrait::NightOwl})
    if ((profile.traitFlags & guestTraitFlag(trait)) != 0 &&
        traitConflicts(profile.traitFlags & ~guestTraitFlag(trait), trait))
      return false;
  return true;
}

bool sameGuestProfile(const GuestProfileView &a, const GuestProfileView &b) {
  return a.archetype == b.archetype &&
         a.budgetPerNightCents == b.budgetPerNightCents &&
         a.priceSensitivity == b.priceSensitivity &&
         a.serviceSensitivity == b.serviceSensitivity &&
         a.cleanlinessSensitivity == b.cleanlinessSensitivity &&
         a.noiseSensitivity == b.noiseSensitivity &&
         a.privacySensitivity == b.privacySensitivity &&
         a.safetySensitivity == b.safetySensitivity &&
         a.comfortSensitivity == b.comfortSensitivity &&
         a.foodSensitivity == b.foodSensitivity && a.patience == b.patience &&
         a.traitFlags == b.traitFlags;
}

void writeGuestProfile(std::ostream &output, const GuestProfileView &profile) {
  output << ei(profile.archetype) << ' ' << profile.budgetPerNightCents << ' '
         << profile.priceSensitivity << ' ' << profile.serviceSensitivity
         << ' ' << profile.cleanlinessSensitivity << ' '
         << profile.noiseSensitivity << ' ' << profile.privacySensitivity
         << ' ' << profile.safetySensitivity << ' '
         << profile.comfortSensitivity << ' ' << profile.foodSensitivity << ' '
         << profile.patience << ' ' << profile.traitFlags;
}

bool readGuestProfile(std::istream &input, GuestProfileView &profile) {
  int archetype{};
  input >> archetype >> profile.budgetPerNightCents >>
      profile.priceSensitivity >> profile.serviceSensitivity >>
      profile.cleanlinessSensitivity >> profile.noiseSensitivity >>
      profile.privacySensitivity >> profile.safetySensitivity >>
      profile.comfortSensitivity >> profile.foodSensitivity >> profile.patience >>
      profile.traitFlags;
  profile.archetype = static_cast<GuestArchetype>(archetype);
  return static_cast<bool>(input) && validGuestProfile(profile);
}

int queueToleranceFor(const GuestProfileView &profile, double baseSeconds = 480) {
  double segmentModifier = 1.0;
  switch (profile.archetype) {
  case GuestArchetype::FamilyLeisure:
    segmentModifier = 1.15;
    break;
  case GuestArchetype::GroupTourTraveler:
    segmentModifier = 1.20;
    break;
  case GuestArchetype::BusinessTraveler:
    segmentModifier = .90;
    break;
  case GuestArchetype::ExecutiveBusiness:
    segmentModifier = .85;
    break;
  case GuestArchetype::AirportTransitTraveler:
    segmentModifier = .70;
    break;
  case GuestArchetype::VipCelebrity:
    segmentModifier = .60;
    break;
  case GuestArchetype::CriticReviewer:
    segmentModifier = .75;
    break;
  default:
    break;
  }
  double traitModifier = 1.0;
  if (profile.traitFlags & guestTraitFlag(GuestTrait::Patient))
    traitModifier *= 1.30;
  if (profile.traitFlags & guestTraitFlag(GuestTrait::Impatient))
    traitModifier *= .65;
  return static_cast<int>(std::clamp(
      std::lround(baseSeconds * (.5 + profile.patience) * segmentModifier *
                  traitModifier),
      120L, 1200L));
}

bool materialsZero(const ConstructionMaterials &materials) {
  return materials == ConstructionMaterials{};
}
} // namespace

struct Simulation::Impl {
  static constexpr std::size_t completedTaskHistoryLimit = 128;

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
  InventoryView inventory{24, 48, 36, 24, 8};
  EconomyView economy{2500000};
  ConstructionSnapshot construction;
  BuildingSystemsSnapshot buildingSystems;
  double baseDemand{1.5}, turnoverWork{2400}, repairWork{1800},
      checkInWork{300}, hungerRate{10.0 / 60.0}, restLoss{5.0 / 60.0},
      roomConditionLossPerDay{2.5};
  int utilityPerRoomDayCents{350};
  bool staffOptimizerEnabled{true};

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
  Room *getRoom(EntityId id) {
    for (auto &x : rooms)
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
  void refreshReachability() {
    for (auto &r : rooms) {
      r.reachable =
          has(TileKind::Entrance) && !path(entrance(), r.door).empty();
      if (!r.closed && r.status == RoomStatus::Incomplete && r.reachable)
        r.status = RoomStatus::VacantReady;
    }
  }
  EntityId createTask(TaskKind kind, EntityId target, Position pos, double work) {
    for (auto &t : tasks)
      if (t.targetId == target && t.kind == kind &&
          t.status != TaskStatus::Completed)
        return t.id;
    Task t;
    t.id = nextId++;
    t.kind = kind;
    t.targetId = target;
    t.target = pos;
    t.total = t.workRemainingSeconds = work;
    tasks.push_back(t);
    return t.id;
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

  void tryReserveBuildMaterials(BuildJobSnapshot &job) {
    if (job.state != BuildJobState::WaitingForMaterials)
      return;
    const auto required = detail::constructionMaterialsFor(job.construction);
    if (!detail::hasMaterials(construction.availableMaterials, required))
      return;
    detail::subtractMaterials(construction.availableMaterials, required);
    detail::addMaterials(construction.reservedMaterials, required);
    job.reservedMaterials = required;
    job.state = BuildJobState::ReadyForLabor;
    const Position target = job.construction.placements.empty()
                                ? entrance()
                                : job.construction.placements.front().origin;
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
    const auto receptionRoute = path(entrance(), frontDesk());
    if (receptionRoute.empty() && !same(entrance(), frontDesk()))
      return;
    const double baseReputationUtility =
        0.2 + 0.8 * std::clamp((economy.reputation - 60.0) / 20.0, 0.0, 1.0);
    constexpr double segmentMarketCalibration = 1.15;
    for (auto &room : rooms) {
      Room *r = &room;
      if (r->closed || r->status != RoomStatus::VacantReady || !r->reachable ||
          !detail::validateRoomSystems(buildingSystems, r->id).sellable)
        continue;
      std::mt19937_64 marketRandom(
          mixedSeed(seed, static_cast<std::uint64_t>(day),
                    static_cast<std::uint64_t>(hour), r->id, 0x4d41524b4554ULL));
      const auto profile = generateGuestProfile(marketRandom);
      const double priceRatio = static_cast<double>(r->nightlyRateCents) /
                                profile.budgetPerNightCents;
      double priceUtility = 0;
      if (priceRatio <= 0.75)
        priceUtility = 1;
      else if (priceRatio <= 1.0)
        priceUtility = 1.0 - (priceRatio - 0.75) * 0.8;
      else if (priceRatio <= 1.25)
        priceUtility = 0.8 - (priceRatio - 1.0) * 2.2;
      else if (priceRatio <= 1.5)
        priceUtility = 0.25 - (priceRatio - 1.25);
      if (priceUtility > 0)
        priceUtility =
            std::pow(priceUtility, .5 + profile.priceSensitivity * 1.5);
      const double reputationUtility = std::pow(
          baseReputationUtility, .5 + profile.serviceSensitivity);
      const double dailyChance =
          std::clamp(baseDemand * segmentMarketCalibration * reputationUtility *
                         priceUtility,
                     0.0, 1.0);
      const double hourlyChance =
          dailyChance >= 1.0 ? 1.0
                             : 1.0 - std::pow(1.0 - dailyChance, 1.0 / 24.0);
      if (randomUnit(marketRandom) > hourlyChance)
        continue;
      Reservation z;
      z.id = nextId++;
      z.guestName = "Guest " + std::to_string(z.id);
      z.roomId = r->id;
      z.arrivalDay = day + (hour > 15 ? 1 : 0);
      z.departureDay = z.arrivalDay + 1 + static_cast<int>(marketRandom() % 3);
      z.nightlyRateCents = r->nightlyRateCents;
      z.nightlyRate = z.nightlyRateCents / 100.0;
      z.profile = profile;
      reservations.push_back(z);
      r->status = RoomStatus::Reserved;
      r->reservationId = z.id;
    }
  }
  void arrivals(int day) {
    if (!has(TileKind::FrontDesk))
      return;
    for (auto &z : reservations)
      if (!z.arrived && !reservationFinished(z) && z.arrivalDay <= day) {
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
        p.queueToleranceSeconds = queueToleranceFor(z.profile);
        p.goal = "Reach front desk";
        p.reservationId = z.id;
        p.profile = z.profile;
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
            r->status = RoomStatus::OutOfOrder;
            createTask(TaskKind::Repair, r->id, r->door, repairWork);
          } else {
            r->status = RoomStatus::VacantDirty;
            createTask(TaskKind::Turnover, r->id, r->door, turnoverWork);
          }
        }
        for (auto &p : people)
          if (p.reservationId == z.id) {
            p.destination = frontDesk();
            p.state = PersonState::Traveling;
            p.queueWaitSeconds = 0;
            p.patience = 100;
            p.queueToleranceSeconds = queueToleranceFor(p.profile, 360);
            p.goal = "Reach front desk for checkout";
          }
      }
  }
  void completeCheckout(Person &guest) {
    auto *z = getReservation(guest.reservationId);
    if (!z || z->completed)
      return;
    auto *r = getRoom(z->roomId);
    z->completed = true;
    economy.completedStays++;
    const auto charge = z->nightlyRateCents * (z->departureDay - z->arrivalDay);
    economy.revenueCents += charge;
    economy.cashCents += charge;
    double scoreValue =
        z->satisfaction + (z->checkoutCleanliness - 80.0) *
                              (.5 + z->profile.cleanlinessSensitivity) -
        ((r && !r->reachable) ? 20.0 : 0.0);
    if (scoreValue < 70 &&
        (z->profile.traitFlags & guestTraitFlag(GuestTrait::Forgiving)))
      scoreValue = 70 + (scoreValue - 70) * .80;
    const int overallSatisfaction =
        static_cast<int>(std::clamp(std::lround(scoreValue), 0L, 100L));
    double reviewChance = .65;
    if (overallSatisfaction <= 40)
      reviewChance += .20;
    if (overallSatisfaction >= 90)
      reviewChance += .10;
    if (z->profile.archetype == GuestArchetype::GroupTourTraveler)
      reviewChance *= .50;
    if (overallSatisfaction < 70 &&
        (z->profile.traitFlags & guestTraitFlag(GuestTrait::ComplaintProne)))
      reviewChance += .15;
    if (z->profile.archetype == GuestArchetype::CriticReviewer)
      reviewChance = 1.0;
    reviewChance = std::clamp(reviewChance, 0.0, 1.0);
    std::mt19937_64 reviewRandom(mixedSeed(
        seed, static_cast<std::uint64_t>(z->arrivalDay),
        static_cast<std::uint64_t>(z->profile.archetype), z->roomId,
        0x524556494557ULL));
    if (randomUnit(reviewRandom) <= reviewChance) {
      const double reviewScore = std::clamp(
          1.0 + overallSatisfaction * .09 +
              (randomUnit(reviewRandom) * .60 - .30),
          1.0, 10.0);
      reviews.push_back(
          {z->id, static_cast<int>(elapsed / 86400), reviewScore,
           reviewScore >= 8.0   ? "A comfortable, well-run stay."
           : reviewScore >= 6.0 ? "Fine, though service could improve."
                                : "Service delays hurt the stay."});
      const double reputationWeight =
          z->profile.archetype == GuestArchetype::CriticReviewer
              ? criticReviewReputationWeight
          : z->profile.archetype == GuestArchetype::VipCelebrity ? .30
                                      : normalReviewReputationWeight;
      economy.reputation = economy.reputation * (1.0 - reputationWeight) +
                           overallSatisfaction * reputationWeight;
    }
    guest.destination = entrance();
    guest.state = PersonState::Traveling;
    guest.goal = "Leave hotel";
  }
  bool abandonCheckIn(Person &guest) {
    auto *reservation = getReservation(guest.reservationId);
    if (!reservation || reservation->checkedIn ||
        reservationFinished(*reservation))
      return false;
    for (const auto &task : tasks)
      if (task.kind == TaskKind::CheckIn && task.targetId == guest.id &&
          task.status == TaskStatus::Working)
        return false;
    for (auto &task : tasks)
      if (task.kind == TaskKind::CheckIn && task.targetId == guest.id &&
          task.status != TaskStatus::Completed) {
        if (auto *employee = getPerson(task.employeeId)) {
          if (employee->task == task.id) {
            employee->task = 0;
            employee->state = PersonState::Idle;
            employee->destination = employee->position;
          }
        }
        task.status = TaskStatus::Completed;
        task.workRemainingSeconds = 0;
        task.blockedReason = "Guest abandoned check-in";
      }
    reservation->walkedRelocated = true;
    reservation->checkInTravelSeconds = guest.travelSeconds;
    reservation->checkInWaitSeconds = guest.queueWaitSeconds;
    guest.satisfaction = std::max(0.0, guest.satisfaction - 20.0);
    reservation->satisfaction = guest.satisfaction;
    if (auto *room = getRoom(reservation->roomId)) {
      if (room->reservationId == reservation->id) {
        room->reservationId = 0;
        if (room->condition < 35) {
          room->status = RoomStatus::OutOfOrder;
          createTask(TaskKind::Repair, room->id, room->door, repairWork);
        } else {
          room->status = room->cleanliness >= 70 ? RoomStatus::VacantReady
                                                 : RoomStatus::VacantDirty;
        }
      }
    }
    economy.reputation = economy.reputation * .90 +
                         std::max(0.0, guest.satisfaction - 25.0) * .10;
    guest.destination = entrance();
    guest.state = PersonState::Traveling;
    guest.goal = "Leave hotel";
    return true;
  }
  bool shiftActive(const Person &p, int hour) const {
    if (p.shiftStartHour == p.shiftEndHour)
      return true;
    if (p.shiftStartHour < p.shiftEndHour)
      return hour >= p.shiftStartHour && hour < p.shiftEndHour;
    return hour >= p.shiftStartHour || hour < p.shiftEndHour;
  }
  bool eligible(const Person &p, TaskKind k) const {
    return (k == TaskKind::Turnover || k == TaskKind::Restock)
               ? p.kind == PersonKind::Housekeeper
           : (k == TaskKind::Repair || k == TaskKind::Build)
               ? p.kind == PersonKind::Maintenance
               : p.kind == PersonKind::Receptionist;
  }
  void postAccruedWage(Person &person) {
    const std::int64_t cents = person.accruedWageUnits / 3600;
    person.accruedWageUnits %= 3600;
    economy.payrollCents += cents;
    economy.cashCents -= cents;
  }
  hh::optimization::TaskState optimizerTaskState(const Task &task) const {
    switch (task.status) {
    case TaskStatus::Ready:
      return hh::optimization::TaskState::Ready;
    case TaskStatus::Blocked:
      return hh::optimization::TaskState::Blocked;
    case TaskStatus::Traveling:
      return hh::optimization::TaskState::Traveling;
    case TaskStatus::Working:
      return hh::optimization::TaskState::Executing;
    case TaskStatus::Completed:
      return hh::optimization::TaskState::Completed;
    }
    return hh::optimization::TaskState::Created;
  }
  int optimizerPriority(TaskKind kind) const {
    switch (kind) {
    case TaskKind::CheckOut:
      return 500;
    case TaskKind::CheckIn:
      return 450;
    case TaskKind::Repair:
      return 300;
    case TaskKind::Build:
      return 250;
    case TaskKind::Turnover:
      return 200;
    case TaskKind::Restock:
      return 100;
    }
    return 0;
  }
  int authoritativeRouteSeconds(Position from, Position to) const {
    if (same(from, to))
      return 0;
    const auto route = path(from, to);
    return route.empty() ? -1 : static_cast<int>(route.size());
  }
  int optimizerTravelSeconds(const Person &person, const Task &task) const {
    if (task.kind == TaskKind::Turnover && !task.resourcesClaimed) {
      const auto closet = supply();
      const int toSupply = authoritativeRouteSeconds(person.position, closet);
      const int toTarget = authoritativeRouteSeconds(closet, task.target);
      return toSupply < 0 || toTarget < 0 ? -1 : toSupply + toTarget;
    }
    return authoritativeRouteSeconds(person.position, task.target);
  }
  bool refreshTaskReadiness(Task &task) {
    bool resources = true;
    if (task.kind == TaskKind::Turnover)
      resources = task.resourcesClaimed ||
                  (has(TileKind::SupplyCloset) && inventory.linen >= 1 &&
                   inventory.towels >= 2 && inventory.amenities >= 1 &&
                   inventory.chemicals >= 1);
    if (task.kind == TaskKind::Repair)
      resources = task.resourcesClaimed || inventory.parts >= 1;
    if (task.kind == TaskKind::CheckIn || task.kind == TaskKind::CheckOut)
      resources = has(TileKind::FrontDesk);
    if (task.kind == TaskKind::Build) {
      const auto *job = getBuildJob(task.targetId);
      resources = job && (job->state == BuildJobState::ReadyForLabor ||
                          job->state == BuildJobState::Building);
    }
    if (!resources) {
      task.status = TaskStatus::Blocked;
      task.blockedReason = "Required local supplies unavailable";
      return false;
    }
    task.blockedReason.clear();
    task.status = TaskStatus::Ready;
    return true;
  }
  void commitStaffAssignment(Task &task, Person &person) {
    task.employeeId = person.id;
    person.task = task.id;
    person.destination =
        (task.kind == TaskKind::Turnover && !task.resourcesClaimed) ? supply()
                                                                   : task.target;
    person.state = PersonState::Traveling;
    task.status = TaskStatus::Traveling;
    if (task.kind == TaskKind::Repair && !task.resourcesClaimed) {
      inventory.parts--;
      task.resourcesClaimed = true;
    }
    if (task.kind == TaskKind::Turnover)
      if (auto *room = getRoom(task.targetId))
        room->status = RoomStatus::Cleaning;
  }
  bool assignNativeGreedy(Task &task) {
    if (task.status != TaskStatus::Ready || !refreshTaskReadiness(task))
      return false;
    Person *best = nullptr;
    int distance = 999999;
    for (auto &person : people)
      if (person.kind != PersonKind::Guest && person.onShift &&
          person.task == 0 && eligible(person, task.kind)) {
        const int candidateDistance = manhattan(person.position, task.target);
        if (candidateDistance < distance) {
          distance = candidateDistance;
          best = &person;
        }
      }
    if (!best)
      return false;
    commitStaffAssignment(task, *best);
    return true;
  }
  hh::optimization::OptimizerSnapshot buildOptimizerSnapshot() const {
    hh::optimization::OptimizerSnapshot snapshot;
    snapshot.optimizationEpoch =
        elapsed < 0 ? 0 : static_cast<std::uint64_t>(elapsed);
    snapshot.simulationSecond = elapsed;
    for (const auto &person : people) {
      if (person.kind == PersonKind::Guest)
        continue;
      hh::optimization::Employee employee;
      employee.id = person.id;
      employee.available = person.onShift && person.task == 0;
      employee.fatigue = static_cast<std::int32_t>(std::lround(person.fatigue));
      employee.regularWageMinorPerHour = person.hourlyWageCents;
      employee.overtimeWageMinorPerHour = person.hourlyWageCents;
      employee.availableFromBucket = 0;
      employee.availableUntilBucket = hh::optimization::kLivePlanningHorizonBuckets;
      snapshot.employees.push_back(employee);
    }
    for (const auto &task : tasks) {
      hh::optimization::Task optimizedTask;
      optimizedTask.id = task.id;
      optimizedTask.state = optimizerTaskState(task);
      optimizedTask.priority = optimizerPriority(task.kind);
      optimizedTask.estimatedWorkSeconds = static_cast<std::int32_t>(
          std::ceil(std::max(0.0, task.workRemainingSeconds)));
      optimizedTask.guestImpactPoints =
          task.kind == TaskKind::CheckOut || task.kind == TaskKind::CheckIn
              ? 100
              : task.kind == TaskKind::Turnover ? 60 : 30;
      optimizedTask.revenueImpactPoints =
          task.kind == TaskKind::Turnover
              ? 80
              : task.kind == TaskKind::Repair ? 70
              : task.kind == TaskKind::Build  ? 55
                                              : 40;
      snapshot.tasks.push_back(optimizedTask);
      if (task.status != TaskStatus::Ready)
        continue;
      for (const auto &person : people) {
        if (person.kind == PersonKind::Guest)
          continue;
        hh::optimization::Candidate candidate;
        candidate.employeeId = person.id;
        candidate.taskId = task.id;
        const int travelSeconds = optimizerTravelSeconds(person, task);
        candidate.eligible = person.onShift && person.task == 0 &&
                             eligible(person, task.kind) && travelSeconds >= 0;
        candidate.travelSeconds = std::max(0, travelSeconds);
        double efficiency = 0.75 + 0.75 * person.skill / 100.0;
        if (person.fatigue > 80)
          efficiency /= 1.25;
        else if (person.fatigue > 60)
          efficiency /= 1.10;
        candidate.effectiveWorkSeconds = static_cast<std::int32_t>(std::ceil(
            std::max(0.0, task.workRemainingSeconds) /
            std::max(0.01, efficiency)));
        candidate.fatiguePenaltySeconds =
            static_cast<std::int32_t>(std::lround(person.fatigue));
        candidate.skillBonusSeconds =
            static_cast<std::int32_t>(std::lround(person.skill));
        snapshot.candidates.push_back(candidate);
      }
    }
    return snapshot;
  }
  bool commitOptimizerPlan(const hh::optimization::SchedulerPlan &plan) {
    struct PendingCommit {
      Task *task{};
      Person *person{};
    };
    std::vector<PendingCommit> commits;
    std::unordered_set<EntityId> employees;
    std::unordered_set<EntityId> assignedTasks;
    int repairPartsNeeded = 0;
    for (const auto &assignment : plan.assignments) {
      if (assignment.startBucket != 0)
        continue;
      auto taskIt = std::find_if(tasks.begin(), tasks.end(), [&](Task &task) {
        return task.id == assignment.taskId;
      });
      auto *person = getPerson(assignment.employeeId);
      if (taskIt == tasks.end() || !person || taskIt->status != TaskStatus::Ready ||
          !person->onShift || person->task != 0 ||
          !eligible(*person, taskIt->kind) ||
          optimizerTravelSeconds(*person, *taskIt) < 0 ||
          !employees.insert(person->id).second ||
          !assignedTasks.insert(taskIt->id).second)
        return false;
      if (taskIt->kind == TaskKind::Repair && !taskIt->resourcesClaimed)
        ++repairPartsNeeded;
      commits.push_back({&*taskIt, person});
    }
    if (repairPartsNeeded > inventory.parts)
      return false;
    for (auto &commit : commits)
      commitStaffAssignment(*commit.task, *commit.person);
    return true;
  }
  void staffAndTasks() {
    int hour = (elapsed / 3600) % 24;
    std::vector<EntityId> repairedRooms;
    std::vector<EntityId> failedRooms;
    for (auto &p : people)
      if (p.kind != PersonKind::Guest) {
        p.onShift = shiftActive(p, hour);
        if (!p.onShift) {
          p.state = PersonState::OffDuty;
          p.fatigue = std::max(0.0, p.fatigue - 12.0 / 3600.0);
          p.task = 0;
        } else {
          p.accruedWageUnits += p.hourlyWageCents;
          if (p.state == PersonState::OffDuty)
            p.state = PersonState::Idle;
        }
      }
    for (auto &task : tasks)
      if (task.status == TaskStatus::Ready || task.status == TaskStatus::Blocked)
        refreshTaskReadiness(task);

    for (auto &task : tasks)
      if (task.status == TaskStatus::Ready && task.kind == TaskKind::CheckOut)
        assignNativeGreedy(task);
    for (auto &task : tasks)
      if (task.status == TaskStatus::Ready && task.kind == TaskKind::CheckIn)
        assignNativeGreedy(task);

    bool optimizerCommitted = false;
    if (staffOptimizerEnabled) {
      const auto snapshot = buildOptimizerSnapshot();
      const auto resolution = detail::resolveStaffPlan(snapshot);
      optimizerCommitted = resolution.valid && commitOptimizerPlan(resolution.plan);
    }
    if (!staffOptimizerEnabled || !optimizerCommitted)
      for (auto &task : tasks)
        if (task.status == TaskStatus::Ready)
          assignNativeGreedy(task);

    for (auto &t : tasks)
      if (t.employeeId && t.status != TaskStatus::Completed &&
          t.status != TaskStatus::Blocked) {
        auto *p = getPerson(t.employeeId);
        if (!p || !p->onShift) {
          t.employeeId = 0;
          t.status = TaskStatus::Ready;
          continue;
        }
        if (p->state == PersonState::Traveling) {
          p->fatigue = std::min(100.0, p->fatigue + 4.0 / 3600.0);
          ++p->travelSeconds;
          auto route = path(p->position, p->destination);
          if (route.empty() && !same(p->position, p->destination)) {
            t.status = TaskStatus::Ready;
            t.employeeId = 0;
            p->task = 0;
            p->state = PersonState::Idle;
            continue;
          }
          if (!route.empty())
            p->position = route.front();
          if (same(p->position, p->destination)) {
            if (t.kind == TaskKind::Turnover &&
                same(p->destination, supply()) && !t.resourcesClaimed) {
              if (inventory.linen < 1 || inventory.towels < 2 ||
                  inventory.amenities < 1 || inventory.chemicals < 1) {
                t.status = TaskStatus::Blocked;
                t.blockedReason = "Required local supplies unavailable";
                t.employeeId = 0;
                p->task = 0;
                p->state = PersonState::Idle;
                continue;
              }
              inventory.linen--;
              inventory.towels -= 2;
              inventory.amenities--;
              inventory.chemicals--;
              t.resourcesClaimed = true;
              p->destination = t.target;
              p->state = PersonState::Traveling;
            } else {
              p->state = PersonState::Working;
              t.status = TaskStatus::Working;
              markBuildStarted(t);
            }
          }
        } else if (p->state == PersonState::Working) {
          const double fatiguePerHour =
              t.kind == TaskKind::Turnover
                  ? 10.0
                  : t.kind == TaskKind::Build
                        ? 8.0
                        : (t.kind == TaskKind::CheckIn ||
                                   t.kind == TaskKind::CheckOut
                               ? 4.0
                               : 6.0);
          p->fatigue = std::min(100.0, p->fatigue + fatiguePerHour / 3600.0);
          double efficiency = 0.75 + 0.75 * p->skill / 100.0;
          if (p->fatigue > 80)
            efficiency /= 1.25;
          else if (p->fatigue > 60)
            efficiency /= 1.10;
          t.workRemainingSeconds -= efficiency;
          if (t.workRemainingSeconds <= 0) {
            t.status = TaskStatus::Completed;
            p->task = 0;
            p->state = PersonState::Idle;
            if (t.kind == TaskKind::CheckIn) {
              if (auto *g = getPerson(t.targetId)) {
                for (auto &z : reservations)
                  if (z.id == g->reservationId) {
                    if (auto *r = getRoom(z.roomId)) {
                      z.checkInTravelSeconds = g->travelSeconds;
                      z.checkInWaitSeconds = g->queueWaitSeconds;
                      g->destination = r->door;
                      g->state = PersonState::Traveling;
                      g->goal = "Reach assigned room";
                      z.checkedIn = true;
                      r->status = RoomStatus::Occupied;
                    }
                  }
              }
            }
            if (t.kind == TaskKind::CheckOut)
              if (auto *g = getPerson(t.targetId))
                completeCheckout(*g);
            if (t.kind == TaskKind::Build)
              completeBuildJob(t);
            if (auto *r = getRoom(t.targetId)) {
              if (t.kind == TaskKind::Turnover && !r->closed) {
                if (r->condition < 35) {
                  r->status = RoomStatus::OutOfOrder;
                  failedRooms.push_back(r->id);
                } else {
                  r->status = RoomStatus::VacantReady;
                  r->cleanliness = std::clamp(
                      70 + p->skill * 0.3 - p->fatigue * 0.1, 0.0, 100.0);
                }
              }
              if (t.kind == TaskKind::Repair) {
                r->condition = 100;
                if (!r->closed) {
                  r->status = RoomStatus::VacantDirty;
                  repairedRooms.push_back(r->id);
                }
              }
            }
          }
        }
      }
    for (EntityId roomId : repairedRooms)
      if (auto *room = getRoom(roomId))
        createTask(TaskKind::Turnover, roomId, room->door, turnoverWork);
    for (EntityId roomId : failedRooms)
      if (auto *room = getRoom(roomId))
        createTask(TaskKind::Repair, roomId, room->door, repairWork);
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
            if (p.queueWaitSeconds > p.queueToleranceSeconds)
              p.satisfaction = std::max(
                  0.0, p.satisfaction -
                           0.6 / 60.0 * p.profile.serviceSensitivity);
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
          if (p.queueWaitSeconds > p.queueToleranceSeconds)
            p.satisfaction =
                std::max(0.0, p.satisfaction -
                                  0.6 / 60.0 * p.profile.serviceSensitivity);
          if (p.goal == "Wait for check-in" &&
              p.queueWaitSeconds > p.queueToleranceSeconds + 10 * 60)
            abandonCheckIn(p);
        }
        for (auto &z : reservations)
          if (z.id == p.reservationId)
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
      if (reservationFinished(reservation))
        completedReservationHistory.push_back(std::move(reservation));
    reservations.erase(
        std::remove_if(reservations.begin(), reservations.end(),
                       [](const auto &reservation) {
                         return reservationFinished(reservation);
                       }),
        reservations.end());
  }
  void minute() {
    elapsed += 1;
    int minuteValue = (elapsed / 60) % 60, hour = (elapsed / 3600) % 24,
        day = elapsed / 86400;
    const bool hourBoundary = (elapsed % 3600) == 0;
    if (hourBoundary)
      hourlyBookings(day, hour);
    if (hourBoundary && hour == 11)
      beginDepartures(day);
    if (hourBoundary && hour == 15)
      arrivals(day);
    if (hourBoundary) {
      for (auto &o : orders)
        if (!o.delivered && o.etaDay <= day) {
          inventory.linen += o.items.linen;
          inventory.towels += o.items.towels;
          inventory.amenities += o.items.amenities;
          inventory.chemicals += o.items.chemicals;
          inventory.parts += o.items.parts;
          o.delivered = true;
        }
    }
    refreshBuildJobs();
    staffAndTasks();
    guests();
    for (auto &elevator : buildingSystems.elevators)
      detail::tickElevator(elevator);
    compactTransientState();
    if (hour == 0 && minuteValue == 0 && hourBoundary) {
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
          if (r.condition < 35 && r.reservationId == 0 &&
              r.status == RoomStatus::VacantReady) {
            r.status = RoomStatus::OutOfOrder;
            newlyFailedRooms.push_back(r.id);
          }
        }
      for (const EntityId roomId : newlyFailedRooms)
        if (auto *room = getRoom(roomId))
          createTask(TaskKind::Repair, roomId, room->door, repairWork);
      economy.distressed = economy.cashCents < 0;
      economy.stars = std::min(5, 1 + economy.completedStays / 15);
    }
    int available = 0, occupied = 0;
    for (auto &r : rooms)
      if (!r.closed && r.status != RoomStatus::Incomplete &&
          detail::validateRoomSystems(buildingSystems, r.id).sellable) {
        available++;
        occupied += r.status == RoomStatus::Occupied;
      }
    economy.occupancy = available ? double(occupied) / available : 0;
  }
};

Simulation::Simulation(std::uint64_t seed, int w, int h, int f)
    : impl_(std::make_unique<Impl>()) {
  if (w < 4 || h < 4 || f < 1)
    throw std::invalid_argument("invalid map dimensions");
  impl_->seed = seed;
  impl_->rng.seed(seed);
  impl_->width = w;
  impl_->height = h;
  impl_->floors = f;
  impl_->map.assign((size_t)w * h * f, TileKind::Empty);
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
                            floor, x, 9, 6, 6, {floor, x, 9}, 1, 1,
                            135.0 + i * 10 + floor * 5});
    }
  s.hireStaff({"Alex", PersonKind::Receptionist, 8, 22, 20});
  s.hireStaff({"Morgan", PersonKind::Housekeeper, 8, 16, 18});
  s.hireStaff({"Casey", PersonKind::Maintenance, 10, 12, 25});
  s.impl_->elapsed = 14 * 3600;
  return s;
}
CommandResult Simulation::buildTile(Position p, TileKind k) {
  if (!impl_->inside(p) || ei(k) < ei(TileKind::Empty) ||
      ei(k) > ei(TileKind::Lobby))
    return {false, "Tile or type is invalid"};
  auto old = impl_->map[impl_->index(p)];
  if (old == TileKind::Entrance && k != TileKind::Entrance)
    return {false, "The hotel entrance cannot be removed"};
  if (k == TileKind::Entrance && old != TileKind::Entrance &&
      impl_->has(TileKind::Entrance))
    return {false, "The hotel already has a main entrance"};
  if (old == TileKind::FrontDesk && k != TileKind::FrontDesk &&
      std::any_of(impl_->people.begin(), impl_->people.end(), [](auto &person) {
        return person.kind == PersonKind::Guest &&
               person.state != PersonState::CheckedOut;
      }))
    return {false, "Reception is required while guests are on property"};
  if (old == TileKind::SupplyCloset && k != TileKind::SupplyCloset &&
      std::any_of(impl_->tasks.begin(), impl_->tasks.end(), [](auto &task) {
        return task.kind == TaskKind::Turnover && !task.resourcesClaimed &&
               task.status != TaskStatus::Completed;
      }))
    return {false, "Supply closet is required by an active turnover"};
  if (!passableKind(k) &&
      std::any_of(impl_->people.begin(), impl_->people.end(),
                  [&](auto &person) { return same(person.position, p); }))
    return {false, "A person is standing on this tile"};
  for (auto &r : impl_->rooms)
    if (p.floor == r.floor && p.x >= r.x && p.x < r.x + r.width &&
        p.y >= r.y && p.y < r.y + r.height)
      return {false, "Use room commands to alter a room"};
  for (const auto &object : impl_->construction.objects) {
    const auto *definition = detail::constructionDefinition(object.typeId);
    if (!definition)
      continue;
    ConstructionPlacement placed{object.typeId, object.origin,
                                 object.rotationQuarterTurns};
    for (const auto cell : detail::constructionFootprint(placed, *definition))
      if (same(cell, p))
        return {false, "Remove placed objects before altering their support"};
  }
  if (old == k)
    return {true, "Tile unchanged"};
  if (impl_->economy.cashCents < 500)
    return {false, "Insufficient cash for construction"};
  impl_->map[impl_->index(p)] = k;
  impl_->economy.cashCents -= 500;
  impl_->economy.constructionCostCents += 500;
  impl_->refreshReachability();
  return {true, "Tile built"};
}
CommandResult Simulation::buildFurnishedRoom(const RoomBlueprint &b) {
  if (b.width < 3 || b.height < 3 || b.beds < 1 || b.baths < 1 ||
      !std::isfinite(b.nightlyRate) || b.nightlyRate <= 0 ||
      b.nightlyRate > 5000)
    return {false,
            "Room requires a 3x3 footprint, bed, bath, and positive rate"};
  if (!impl_->inside({b.floor, b.x, b.y}) ||
      !impl_->inside({b.floor, b.x + b.width - 1, b.y + b.height - 1}))
    return {false, "Room footprint outside property"};
  if (b.door.floor != b.floor || b.door.x < b.x ||
      b.door.x >= b.x + b.width || b.door.y < b.y ||
      b.door.y >= b.y + b.height ||
      (b.door.x != b.x && b.door.x != b.x + b.width - 1 &&
       b.door.y != b.y && b.door.y != b.y + b.height - 1))
    return {false, "Door must lie on room perimeter"};
  for (auto &r : impl_->rooms)
    if (r.floor == b.floor && b.x < r.x + r.width && b.x + b.width > r.x &&
        b.y < r.y + r.height && b.y + b.height > r.y)
      return {false, "Room overlaps another room"};
  for (int y = b.y; y < b.y + b.height; ++y)
    for (int x = b.x; x < b.x + b.width; ++x)
      if (impl_->map[impl_->index({b.floor, x, y})] != TileKind::Empty)
        return {false, "Room footprint contains existing construction"};
  const auto cost = static_cast<std::int64_t>(b.width) * b.height * 15000;
  if (impl_->economy.cashCents < cost)
    return {false, "Insufficient cash for furnished room construction"};
  for (int y = b.y; y < b.y + b.height; ++y)
    for (int x = b.x; x < b.x + b.width; ++x) {
      Position p{b.floor, x, y};
      const bool edge = x == b.x || x == b.x + b.width - 1 || y == b.y ||
                        y == b.y + b.height - 1;
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
  impl_->initializeLegacyRoomSystems(r.id);
  impl_->economy.cashCents -= cost;
  impl_->economy.constructionCostCents += cost;
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
  impl_->people.push_back(p);
  return {true, "Staff hired", p.id};
}
CommandResult Simulation::fireStaff(EntityId id) {
  auto it =
      std::find_if(impl_->people.begin(), impl_->people.end(), [&](auto &p) {
        return p.id == id && p.kind != PersonKind::Guest;
      });
  if (it == impl_->people.end())
    return {false, "Employee not found"};
  for (auto &t : impl_->tasks)
    if (t.employeeId == id && t.status != TaskStatus::Completed) {
      t.employeeId = 0;
      t.status = TaskStatus::Ready;
    }
  impl_->postAccruedWage(*it);
  impl_->people.erase(it);
  return {true, "Staff released", id};
}
CommandResult Simulation::setStaffShift(EntityId id, int a, int b) {
  auto *p = impl_->getPerson(id);
  if (!p || p->kind == PersonKind::Guest || a < 0 || a > 23 || b < 0 ||
      b > 23)
    return {false, "Invalid employee or shift"};
  p->shiftStartHour = a;
  p->shiftEndHour = b;
  return {true, "Shift updated", id};
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
  r->status = RoomStatus::VacantDirty;
  impl_->createTask(TaskKind::Turnover, id, r->door, impl_->turnoverWork);
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
  r->status = RoomStatus::OutOfOrder;
  impl_->createTask(TaskKind::Repair, id, r->door, impl_->repairWork);
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
  PendingOrder p;
  p.id = impl_->nextId++;
  p.items = {o.linen, o.towels, o.amenities, o.chemicals, o.parts};
  p.etaDay = impl_->elapsed / 86400 + 2;
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
      !integer("initialLinen", d.inventory.linen) ||
      !integer("initialTowels", d.inventory.towels) ||
      !integer("initialAmenities", d.inventory.amenities) ||
      !integer("initialChemicals", d.inventory.chemicals) ||
      !integer("initialParts", d.inventory.parts) || d.baseDemand < 0 ||
      d.turnoverWork <= 0 || d.repairWork <= 0 || d.checkInWork <= 0 ||
      d.roomConditionLossPerDay < 0 || d.roomConditionLossPerDay > 100)
    return {false, "Definition values are invalid"};
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
    impl_->minute();
  }
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
  v.inventory = impl_->inventory;
  v.economy = impl_->economy;
  for (auto &o : impl_->orders)
    v.supplyOrders.push_back(o);
  return v;
}

std::string Simulation::save() const {
  std::ostringstream o;
  o << std::setprecision(17) << "HHGS 11 " << impl_->seed << ' ' << impl_->width
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
  for (auto &p : impl_->people) {
    o << p.id << ' ' << std::quoted(p.name) << ' ' << ei(p.kind) << ' '
      << ei(p.state) << ' ' << p.position.floor << ' ' << p.position.x << ' '
      << p.position.y << ' ' << p.destination.floor << ' ' << p.destination.x
      << ' ' << p.destination.y << ' ' << p.fatigue << ' ' << p.satisfaction
      << ' ' << p.hunger << ' ' << p.rest << ' ' << p.patience << ' ' << p.skill
      << ' ' << p.shiftStartHour << ' ' << p.shiftEndHour << ' '
      << p.travelSeconds << ' ' << p.queueWaitSeconds << ' '
      << std::quoted(p.goal) << ' ' << p.onShift << ' ' << p.hourlyWageCents
      << ' ' << p.reservationId << ' ' << p.task << ' ' << p.accruedWageUnits
      << ' ' << p.queueToleranceSeconds << ' ';
    writeGuestProfile(o, p.profile);
    o << '\n';
  }
  o << impl_->reservations.size() + impl_->completedReservationHistory.size()
    << '\n';
  auto writeReservation = [&](const Reservation &r) {
    o << r.id << ' ' << std::quoted(r.guestName) << ' ' << r.roomId << ' '
      << r.arrivalDay << ' ' << r.departureDay << ' ' << r.nightlyRateCents
      << ' ' << r.checkedIn << ' ' << r.completed << ' ' << r.satisfaction
      << ' ' << r.arrived << ' ' << r.checkoutStarted << ' '
      << r.checkoutCleanliness << ' ';
    writeGuestProfile(o, r.profile);
    o << ' ' << r.walkedRelocated << ' ' << r.checkInTravelSeconds << ' '
      << r.checkInWaitSeconds << '\n';
  };
  for (auto &r : impl_->reservations)
    writeReservation(r);
  for (auto &r : impl_->completedReservationHistory)
    writeReservation(r);
  o << impl_->tasks.size() + impl_->completedTaskHistory.size() << '\n';
  auto writeTask = [&](const Task &t) {
    o << t.id << ' ' << ei(t.kind) << ' ' << ei(t.status) << ' ' << t.targetId
      << ' ' << t.employeeId << ' ' << t.target.floor << ' ' << t.target.x
      << ' ' << t.target.y << ' ' << t.workRemainingSeconds << ' '
      << std::quoted(t.blockedReason) << ' ' << t.total << ' '
      << t.resourcesClaimed << '\n';
  };
  for (auto &t : impl_->tasks)
    writeTask(t);
  for (auto &t : impl_->completedTaskHistory)
    writeTask(t);
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

  const auto writeMaterials = [&](const ConstructionMaterials &materials) {
    o << materials.lumber << ' ' << materials.drywall << ' '
      << materials.electrical << ' ' << materials.plumbing << ' '
      << materials.hardware;
  };
  o << "FINAL01\n";
  o << impl_->construction.objects.size() << '\n';
  for (const auto &object : impl_->construction.objects)
    o << object.id << ' ' << std::quoted(object.typeId) << ' '
      << object.origin.floor << ' ' << object.origin.x << ' ' << object.origin.y
      << ' ' << object.width << ' ' << object.height << ' '
      << object.rotationQuarterTurns << ' ' << object.blocksMovement << '\n';
  writeMaterials(impl_->construction.availableMaterials);
  o << '\n';
  writeMaterials(impl_->construction.reservedMaterials);
  o << '\n' << impl_->construction.buildJobs.size() << '\n';
  for (const auto &job : impl_->construction.buildJobs) {
    o << job.id << ' ' << ei(job.state) << ' ' << job.reservedCashCents << ' '
      << job.workSeconds << ' ' << job.materialsConsumed << ' ' << job.taskId
      << ' ' << std::quoted(job.blockedReason) << ' ';
    writeMaterials(job.reservedMaterials);
    o << ' ' << job.construction.placements.size();
    for (const auto &placement : job.construction.placements)
      o << ' ' << std::quoted(placement.typeId) << ' ' << placement.origin.floor
        << ' ' << placement.origin.x << ' ' << placement.origin.y << ' '
        << placement.rotationQuarterTurns;
    o << ' ' << job.construction.removeObjectIds.size();
    for (const auto id : job.construction.removeObjectIds)
      o << ' ' << id;
    o << '\n';
  }

  o << impl_->buildingSystems.utilityNodes.size() << '\n';
  for (const auto &node : impl_->buildingSystems.utilityNodes)
    o << node.id << ' ' << ei(node.kind) << ' ' << node.roomId << ' '
      << node.source << ' ' << node.capacity << ' ' << node.load << '\n';
  o << impl_->buildingSystems.utilityEdges.size() << '\n';
  for (const auto &edge : impl_->buildingSystems.utilityEdges)
    o << edge.from << ' ' << edge.to << '\n';
  o << impl_->buildingSystems.rooms.size() << '\n';
  for (const auto &room : impl_->buildingSystems.rooms)
    o << room.roomId << ' ' << room.egress << ' ' << room.accessible << ' '
      << room.fireCovered << ' ' << room.securityCovered << '\n';
  o << impl_->buildingSystems.elevators.size() << '\n';
  for (const auto &elevator : impl_->buildingSystems.elevators) {
    o << elevator.id << ' ' << ei(elevator.kind) << ' ' << elevator.minFloor
      << ' ' << elevator.maxFloor << ' ' << elevator.currentFloor << ' '
      << elevator.targetFloor << ' ' << elevator.capacity << ' '
      << elevator.travelSecondsPerFloor << ' ' << elevator.doorSeconds << ' '
      << ei(elevator.state) << ' ' << elevator.phaseSecondsRemaining << ' '
      << elevator.activeRequestId << ' ' << elevator.requests.size();
    for (const auto &request : elevator.requests)
      o << ' ' << request.id << ' ' << request.pickupFloor << ' '
        << request.destinationFloor << ' ' << request.boarded;
    o << '\n';
  }
  return o.str();
}

Simulation Simulation::load(std::string_view data) {
  if (data.size() > 64 * 1024 * 1024)
    throw std::invalid_argument("simulation save too large");
  std::istringstream i{std::string(data)};
  std::string magic;
  int version, w, h, f;
  i >> magic >> version;
  if (magic != "HHGS" || version < 2 || version > 11)
    throw std::invalid_argument("unsupported simulation save");
  std::uint64_t seed;
  i >> seed >> w >> h >> f;
  if (w < 4 || w > 512 || h < 4 || h > 512 || f < 1 || f > 64)
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
        !d.inside({r.floor, r.x, r.y}) ||
        !d.inside({r.floor, r.x + r.width - 1, r.y + r.height - 1}) ||
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
    i >> p.reservationId >> p.task;
    if (version >= 6)
      i >> p.accruedWageUnits;
    else
      i >> legacyAccruedWageMicros;
    if (version >= 8) {
      i >> p.queueToleranceSeconds;
      if (!readGuestProfile(i, p.profile))
        throw std::invalid_argument("invalid saved guest profile");
    } else {
      p.profile = GuestProfileView{};
      p.queueToleranceSeconds =
          k == ei(PersonKind::Guest) ? queueToleranceFor(p.profile) : 0;
    }
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
        p.hourlyWageCents > 1000000 || p.accruedWageUnits < 0 ||
        !validGuestProfile(p.profile) ||
        (k == ei(PersonKind::Guest)
             ? (p.queueToleranceSeconds < 120 ||
                p.queueToleranceSeconds > 1200)
             : p.queueToleranceSeconds != 0))
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
    if (version >= 8) {
      if (!readGuestProfile(i, r.profile))
        throw std::invalid_argument("invalid saved guest profile");
      i >> r.walkedRelocated;
    } else {
      r.profile = GuestProfileView{};
      r.walkedRelocated = false;
    }
    if (version >= 10)
      i >> r.checkInTravelSeconds >> r.checkInWaitSeconds;
    else {
      r.checkInTravelSeconds = 0;
      r.checkInWaitSeconds = 0;
    }
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
        r.checkoutCleanliness < 0 || r.checkoutCleanliness > 100 ||
        r.checkInTravelSeconds < 0 || r.checkInWaitSeconds < 0 ||
        (!r.arrived &&
         (r.checkInTravelSeconds != 0 || r.checkInWaitSeconds != 0)) ||
        !validGuestProfile(r.profile) || (r.completed && r.walkedRelocated) ||
        (r.walkedRelocated &&
         (!r.arrived || r.checkedIn || r.checkoutStarted)))
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
    const int maxTaskKind =
        version >= 11 ? ei(TaskKind::Build) : ei(TaskKind::CheckOut);
    if (k < ei(TaskKind::CheckIn) || k > maxTaskKind ||
        st < ei(TaskStatus::Ready) || st > ei(TaskStatus::Completed) ||
        !d.inside(t.target) || !std::isfinite(t.workRemainingSeconds) ||
        !std::isfinite(t.total))
      throw std::invalid_argument("invalid saved task");
    t.kind = static_cast<TaskKind>(k);
    t.status = static_cast<TaskStatus>(st);
  }
  i >> n;
  if (n > 100000)
    throw std::invalid_argument("too many saved reviews");
  d.reviews.resize(n);
  for (auto &r : d.reviews) {
    i >> r.reservationId >> r.day >> r.score >> std::quoted(r.text);
    if (version <= 8) {
      if (!i || !std::isfinite(r.score) || r.score < 0 || r.score > 100)
        throw std::invalid_argument("invalid legacy saved review");
      r.score = std::clamp(1.0 + r.score * .09, 1.0, 10.0);
    }
  }
  for (const auto &r : d.reviews)
    if (r.day < 0 || !std::isfinite(r.score) || r.score < 1 || r.score > 10)
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
  if (!i)
    throw std::invalid_argument("corrupt simulation save");

  auto readMaterials = [&](ConstructionMaterials &materials) {
    i >> materials.lumber >> materials.drywall >> materials.electrical >>
        materials.plumbing >> materials.hardware;
    return static_cast<bool>(i) && detail::validMaterials(materials);
  };
  bool hasFinal01 = version >= 11;
  if (!hasFinal01) {
    i >> std::ws;
    if (!i.eof()) {
      std::string marker;
      i >> marker;
      if (marker != "FINAL01")
        throw std::invalid_argument("unexpected trailing save data");
      hasFinal01 = true;
    }
  } else {
    std::string marker;
    i >> marker;
    if (marker != "FINAL01")
      throw std::invalid_argument("missing FINAL-01 save state");
  }
  if (hasFinal01) {
    i >> n;
    if (n > 100000)
      throw std::invalid_argument("too many construction objects");
    d.construction.objects.resize(n);
    for (auto &object : d.construction.objects) {
      i >> object.id >> std::quoted(object.typeId) >> object.origin.floor >>
          object.origin.x >> object.origin.y >> object.width >> object.height >>
          object.rotationQuarterTurns >> object.blocksMovement;
      const auto *definition = detail::constructionDefinition(object.typeId);
      if (!i || !definition || !d.inside(object.origin) || object.width < 1 ||
          object.height < 1 || object.rotationQuarterTurns < 0 ||
          object.rotationQuarterTurns > 3)
        throw std::invalid_argument("invalid construction object");
    }
    if (!readMaterials(d.construction.availableMaterials) ||
        !readMaterials(d.construction.reservedMaterials))
      throw std::invalid_argument("invalid construction materials");
    i >> n;
    if (n > 100000)
      throw std::invalid_argument("too many build jobs");
    d.construction.buildJobs.resize(n);
    for (auto &job : d.construction.buildJobs) {
      int state;
      size_t placements{}, removals{};
      i >> job.id >> state >> job.reservedCashCents >> job.workSeconds >>
          job.materialsConsumed >> job.taskId >> std::quoted(job.blockedReason);
      if (!readMaterials(job.reservedMaterials))
        throw std::invalid_argument("invalid reserved build materials");
      i >> placements;
      if (placements > 100000)
        throw std::invalid_argument("too many build placements");
      job.construction.placements.resize(placements);
      for (auto &placement : job.construction.placements)
        i >> std::quoted(placement.typeId) >> placement.origin.floor >>
            placement.origin.x >> placement.origin.y >>
            placement.rotationQuarterTurns;
      i >> removals;
      if (removals > 100000)
        throw std::invalid_argument("too many build removals");
      job.construction.removeObjectIds.resize(removals);
      for (auto &id : job.construction.removeObjectIds)
        i >> id;
      if (!i || state < ei(BuildJobState::WaitingForMaterials) ||
          state > ei(BuildJobState::Cancelled) || job.reservedCashCents < 0 ||
          job.workSeconds <= 0 || job.workSeconds > 7 * 86400)
        throw std::invalid_argument("invalid build job");
      job.state = static_cast<BuildJobState>(state);
    }

    i >> n;
    if (n > 100000)
      throw std::invalid_argument("too many utility nodes");
    d.buildingSystems.utilityNodes.resize(n);
    for (auto &node : d.buildingSystems.utilityNodes) {
      int kind;
      i >> node.id >> kind >> node.roomId >> node.source >> node.capacity >>
          node.load;
      if (!i || kind < ei(UtilityKind::Power) || kind > ei(UtilityKind::Water) ||
          node.capacity < 0 || node.load < 0)
        throw std::invalid_argument("invalid utility node");
      node.kind = static_cast<UtilityKind>(kind);
    }
    i >> n;
    if (n > 200000)
      throw std::invalid_argument("too many utility edges");
    d.buildingSystems.utilityEdges.resize(n);
    for (auto &edge : d.buildingSystems.utilityEdges)
      i >> edge.from >> edge.to;
    i >> n;
    if (n > 100000)
      throw std::invalid_argument("too many room systems");
    d.buildingSystems.rooms.resize(n);
    for (auto &room : d.buildingSystems.rooms)
      i >> room.roomId >> room.egress >> room.accessible >> room.fireCovered >>
          room.securityCovered;
    i >> n;
    if (n > 10000)
      throw std::invalid_argument("too many elevators");
    d.buildingSystems.elevators.resize(n);
    for (auto &elevator : d.buildingSystems.elevators) {
      int kind, state;
      size_t requests{};
      i >> elevator.id >> kind >> elevator.minFloor >> elevator.maxFloor >>
          elevator.currentFloor >> elevator.targetFloor >> elevator.capacity >>
          elevator.travelSecondsPerFloor >> elevator.doorSeconds >> state >>
          elevator.phaseSecondsRemaining >> elevator.activeRequestId >> requests;
      if (!i || kind < ei(ElevatorKind::Passenger) ||
          kind > ei(ElevatorKind::Service) || state < ei(ElevatorState::Idle) ||
          state > ei(ElevatorState::Alighting) || elevator.minFloor < 0 ||
          elevator.maxFloor >= f || elevator.minFloor > elevator.maxFloor ||
          elevator.currentFloor < elevator.minFloor ||
          elevator.currentFloor > elevator.maxFloor ||
          elevator.targetFloor < elevator.minFloor ||
          elevator.targetFloor > elevator.maxFloor || elevator.capacity < 1 ||
          elevator.travelSecondsPerFloor < 1 || elevator.doorSeconds < 1 ||
          elevator.phaseSecondsRemaining < 0 || requests > 100000)
        throw std::invalid_argument("invalid elevator state");
      elevator.kind = static_cast<ElevatorKind>(kind);
      elevator.state = static_cast<ElevatorState>(state);
      elevator.requests.resize(requests);
      for (auto &request : elevator.requests) {
        i >> request.id >> request.pickupFloor >> request.destinationFloor >>
            request.boarded;
        if (!i || request.pickupFloor < elevator.minFloor ||
            request.pickupFloor > elevator.maxFloor ||
            request.destinationFloor < elevator.minFloor ||
            request.destinationFloor > elevator.maxFloor)
          throw std::invalid_argument("invalid elevator request");
      }
    }
    detail::refreshRoomUtilityFlags(d.buildingSystems);
  } else {
    for (const auto &room : d.rooms)
      d.initializeLegacyRoomSystems(room.id);
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
  std::unordered_map<EntityId, const BuildJobSnapshot *> buildJobById;
  std::unordered_set<EntityId> utilityNodeIds;
  for (const auto &room : d.rooms) {
    addEntityId(room.id);
    roomById.emplace(room.id, &room);
    if (room.floor != room.door.floor ||
        d.map[d.index(room.door)] != TileKind::Door || room.cleanliness < 0 ||
        room.cleanliness > 100 || room.condition < 0 || room.condition > 100 ||
        (room.closed && room.status != RoomStatus::OutOfOrder))
      throw std::invalid_argument("invalid saved room state");
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
  for (const auto &object : d.construction.objects) {
    addEntityId(object.id);
    const auto *definition = detail::constructionDefinition(object.typeId);
    if (!definition)
      throw std::invalid_argument("invalid construction object type");
    const ConstructionPlacement placement{object.typeId, object.origin,
                                          object.rotationQuarterTurns};
    for (const auto cell : detail::constructionFootprint(placement, *definition))
      if (!d.inside(cell))
        throw std::invalid_argument("invalid construction object footprint");
  }
  for (const auto &job : d.construction.buildJobs) {
    addEntityId(job.id);
    buildJobById.emplace(job.id, &job);
  }
  for (const auto &node : d.buildingSystems.utilityNodes) {
    addEntityId(node.id);
    utilityNodeIds.insert(node.id);
    if (!node.source && !roomById.contains(node.roomId))
      throw std::invalid_argument("utility node references missing room");
  }
  for (const auto &edge : d.buildingSystems.utilityEdges)
    if (!utilityNodeIds.contains(edge.from) || !utilityNodeIds.contains(edge.to))
      throw std::invalid_argument("utility edge references missing node");
  std::unordered_set<EntityId> roomSystemIds;
  for (const auto &system : d.buildingSystems.rooms)
    if (!roomById.contains(system.roomId) ||
        !roomSystemIds.insert(system.roomId).second)
      throw std::invalid_argument("invalid room building-system reference");
  for (const auto &room : d.rooms)
    if (!roomSystemIds.contains(room.id))
      throw std::invalid_argument("room is missing building-system state");
  for (const auto &elevator : d.buildingSystems.elevators) {
    addEntityId(elevator.id);
    std::unordered_set<EntityId> requestIds;
    for (const auto &request : elevator.requests) {
      addEntityId(request.id);
      if (!requestIds.insert(request.id).second)
        throw std::invalid_argument("duplicate elevator request");
    }
    if (elevator.activeRequestId != 0 &&
        !requestIds.contains(elevator.activeRequestId))
      throw std::invalid_argument("invalid active elevator request");
  }

  std::unordered_map<EntityId, int> guestsByReservation;
  for (const auto &person : d.people) {
    if (person.kind == PersonKind::Guest) {
      if (!reservationById.contains(person.reservationId) || person.task != 0)
        throw std::invalid_argument("invalid saved guest references");
      ++guestsByReservation[person.reservationId];
      if (person.state == PersonState::CheckedOut &&
          !reservationFinished(*reservationById.at(person.reservationId)))
        throw std::invalid_argument("invalid saved guest lifecycle");
      if (!sameGuestProfile(person.profile,
                            reservationById.at(person.reservationId)->profile))
        throw std::invalid_argument("mismatched saved guest profile");
    } else {
      if (person.reservationId != 0)
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
    const bool finished = reservationFinished(reservation);
    if ((!finished && room == roomById.end()) ||
        (reservation.checkedIn && !reservation.arrived) ||
        (reservation.checkoutStarted && !reservation.checkedIn) ||
        (reservation.completed && !reservation.checkoutStarted) ||
        (!finished &&
         guestsByReservation[reservation.id] != (reservation.arrived ? 1 : 0)) ||
        (finished && guestsByReservation[reservation.id] > 1))
      throw std::invalid_argument("invalid saved reservation references");
    if (!reservation.checkoutStarted && !finished &&
        room->second->reservationId != reservation.id)
      throw std::invalid_argument("invalid saved room assignment");
  }
  for (const auto &room : d.rooms) {
    if (room.reservationId != 0) {
      const auto reservation = reservationById.find(room.reservationId);
      if (reservation == reservationById.end() ||
          reservation->second->roomId != room.id ||
          reservation->second->checkoutStarted ||
          reservationFinished(*reservation->second))
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
    const bool roomTask =
        task.kind == TaskKind::Turnover || task.kind == TaskKind::Repair;
    const bool buildTask = task.kind == TaskKind::Build;
    if (active &&
        ((guestTask && !personById.contains(task.targetId)) ||
         (roomTask && !roomById.contains(task.targetId)) ||
         (buildTask && !buildJobById.contains(task.targetId))))
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
  for (const auto &job : d.construction.buildJobs) {
    if (job.taskId != 0) {
      const auto task = taskById.find(job.taskId);
      if (task == taskById.end() || task->second->kind != TaskKind::Build ||
          task->second->targetId != job.id)
        throw std::invalid_argument("invalid saved build task reference");
    }
    if ((job.state == BuildJobState::ReadyForLabor ||
         job.state == BuildJobState::Building) &&
        job.taskId == 0)
      throw std::invalid_argument("active build job has no task");
  }
  for (const auto &review : d.reviews) {
    const auto reservation = reservationById.find(review.reservationId);
    if (reservation == reservationById.end() || !reservation->second->completed)
      throw std::invalid_argument("invalid saved review reference");
  }
  detail::refreshRoomUtilityFlags(d.buildingSystems);
  d.compactTransientState();
  return s;
}

} // namespace hh::game
