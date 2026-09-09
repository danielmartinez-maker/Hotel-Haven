#include "hh/game/Simulation.h"
#include "hh/assets/Json.h"
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
  double baseDemand{1.5}, turnoverWork{2400}, repairWork{1800},
      checkInWork{300}, hungerRate{10.0 / 60.0}, restLoss{5.0 / 60.0},
      roomConditionLossPerDay{2.5};
  int utilityPerRoomDayCents{350};

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
  void createTask(TaskKind kind, EntityId target, Position pos, double work) {
    for (auto &t : tasks)
      if (t.targetId == target && t.kind == kind &&
          t.status != TaskStatus::Completed)
        return;
    Task t;
    t.id = nextId++;
    t.kind = kind;
    t.targetId = target;
    t.target = pos;
    t.total = t.workRemainingSeconds = work;
    tasks.push_back(t);
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
      if (r->closed || r->status != RoomStatus::VacantReady || !r->reachable)
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
           : k == TaskKind::Repair ? p.kind == PersonKind::Maintenance
                                   : p.kind == PersonKind::Receptionist;
  }
  void postAccruedWage(Person &person) {
    const std::int64_t cents = person.accruedWageUnits / 3600;
    person.accruedWageUnits %= 3600;
    economy.payrollCents += cents;
    economy.cashCents -= cents;
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
    for (auto &t : tasks)
      if (t.status == TaskStatus::Ready || t.status == TaskStatus::Blocked) {
        bool resources = true;
        if (t.kind == TaskKind::Turnover)
          resources = t.resourcesClaimed ||
                      (has(TileKind::SupplyCloset) && inventory.linen >= 1 &&
                       inventory.towels >= 2 && inventory.amenities >= 1 &&
                       inventory.chemicals >= 1);
        if (t.kind == TaskKind::Repair)
          resources = t.resourcesClaimed || inventory.parts >= 1;
        if (t.kind == TaskKind::CheckIn || t.kind == TaskKind::CheckOut)
          resources = has(TileKind::FrontDesk);
        if (!resources) {
          t.status = TaskStatus::Blocked;
          t.blockedReason = "Required local supplies unavailable";
          continue;
        }
        t.blockedReason.clear();
        t.status = TaskStatus::Ready;
        Person *best = nullptr;
        int dist = 999999;
        for (auto &p : people)
          if (p.onShift && p.task == 0 && eligible(p, t.kind)) {
            int d = manhattan(p.position, t.target);
            if (d < dist) {
              dist = d;
              best = &p;
            }
          }
        if (best) {
          t.employeeId = best->id;
          best->task = t.id;
          best->destination =
              (t.kind == TaskKind::Turnover && !t.resourcesClaimed) ? supply()
                                                                    : t.target;
          best->state = PersonState::Traveling;
          t.status = TaskStatus::Traveling;
          if (t.kind == TaskKind::Repair && !t.resourcesClaimed) {
            inventory.parts--;
            t.resourcesClaimed = true;
          }
          if (t.kind == TaskKind::Turnover)
            if (auto *r = getRoom(t.targetId))
              r->status = RoomStatus::Cleaning;
        }
      }
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
            }
          }
        } else if (p->state == PersonState::Working) {
          const double fatiguePerHour =
              t.kind == TaskKind::Turnover
                  ? 10.0
                  : (t.kind == TaskKind::CheckIn || t.kind == TaskKind::CheckOut
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
        if (!o.delivered && o.etaDay <= day) {
          inventory.linen += o.items.linen;
          inventory.towels += o.items.towels;
          inventory.amenities += o.items.amenities;
          inventory.chemicals += o.items.chemicals;
          inventory.parts += o.items.parts;
          o.delivered = true;
        }
    }
    staffAndTasks();
    guests();
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
      if (!r.closed && r.status != RoomStatus::Incomplete) {
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
    if (p.floor == r.floor && p.x >= r.x && p.x < r.x + r.width && p.y >= r.y &&
        p.y < r.y + r.height)
      return {false, "Use room commands to alter a room"};
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
  if (b.door.floor != b.floor || b.door.x < b.x || b.door.x >= b.x + b.width ||
      b.door.y < b.y || b.door.y >= b.y + b.height ||
      (b.door.x != b.x && b.door.x != b.x + b.width - 1 && b.door.y != b.y &&
       b.door.y != b.y + b.height - 1))
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
      bool edge = x == b.x || x == b.x + b.width - 1 || y == b.y ||
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
  if (!p || p->kind == PersonKind::Guest || a < 0 || a > 23 || b < 0 || b > 23)
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
  o << std::setprecision(17) << "HHGS 10 " << impl_->seed << ' ' << impl_->width
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
  for (auto &r : impl_->reservations) {
    o << r.id << ' ' << std::quoted(r.guestName) << ' ' << r.roomId << ' '
      << r.arrivalDay << ' ' << r.departureDay << ' ' << r.nightlyRateCents
      << ' ' << r.checkedIn << ' ' << r.completed << ' ' << r.satisfaction
      << ' ' << r.arrived << ' ' << r.checkoutStarted << ' '
      << r.checkoutCleanliness << ' ';
    writeGuestProfile(o, r.profile);
    o << ' ' << r.walkedRelocated << ' ' << r.checkInTravelSeconds << ' '
      << r.checkInWaitSeconds << '\n';
  }
  for (auto &r : impl_->completedReservationHistory) {
    o << r.id << ' ' << std::quoted(r.guestName) << ' ' << r.roomId << ' '
      << r.arrivalDay << ' ' << r.departureDay << ' ' << r.nightlyRateCents
      << ' ' << r.checkedIn << ' ' << r.completed << ' ' << r.satisfaction
      << ' ' << r.arrived << ' ' << r.checkoutStarted << ' '
      << r.checkoutCleanliness << ' ';
    writeGuestProfile(o, r.profile);
    o << ' ' << r.walkedRelocated << ' ' << r.checkInTravelSeconds << ' '
      << r.checkInWaitSeconds << '\n';
  }
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
  return o.str();
}
Simulation Simulation::load(std::string_view data) {
  if (data.size() > 64 * 1024 * 1024)
    throw std::invalid_argument("simulation save too large");
  std::istringstream i{std::string(data)};
  std::string magic;
  int version, w, h, f;
  i >> magic >> version;
  if (magic != "HHGS" || version < 2 || version > 10)
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
    if (k < ei(TaskKind::CheckIn) || k > ei(TaskKind::CheckOut) ||
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

  std::unordered_map<EntityId, int> guestsByReservation;
  for (const auto &person : d.people) {
    if (person.kind == PersonKind::Guest) {
      if (!reservationById.contains(person.reservationId) || person.task != 0)
        throw std::invalid_argument("invalid saved guest references");
      ++guestsByReservation[person.reservationId];
      if (person.state == PersonState::CheckedOut &&
          !reservationFinished(*reservationById.at(person.reservationId)))
        throw std::invalid_argument("invalid saved guest lifecycle");
      if (!sameGuestProfile(
              person.profile,
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
