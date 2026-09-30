Warning: truncated output (original token count: 43812)
Total output lines: 3891

#include "hh/game/Simulation.h"
#include "hh/assets/Json.h"
#include <algorithm>
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
template <class Engine>
double deterministicUnitDouble(Engine &rng) {
  // mt19937_64 is specified; map one draw to [0, 1) with a fixed 53-bit
  // conversion so standard-library distribution implementations cannot alter
  // the authoritative simulation stream across platforms.
  constexpr double inverseTwoTo53 = 1.0 / 9007199254740992.0;
  return static_cast<double>(rng() >> 11) * inverseTwoTo53;
}

template <class T, class Engine>
void deterministicShuffle(std::vector<T> &values, Engine &rng) {
  // Explicit Fisher-Yates. Index selection and engine consumption are fixed by
  // this code rather than by std::shuffle's implementation.
  for (std::size_t remaining = values.size(); remaining > 1; --remaining) {
    const auto index = static_cast<std::size_t>(
        rng() % static_cast<typename Engine::result_type>(remaining));
    std::swap(values[remaining - 1], values[index]);
  }
}
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
struct Room : RoomView {};
struct Person : PersonView {
  EntityId reservation{};
  EntityId task{};
  std::int64_t accruedWageUnits{};
  int shiftWorkedSeconds{};
  std::int64_t shiftInstanceKey{std::numeric_limits<std::int64_t>::min()};
  bool breakTaskCreated{};
};
struct GuestMember {
  GuestProfile profile;
  GuestLifecycleState lifecycle{GuestLifecycleState::Prospective};
  GuestNeedState needs;
  GuestExperienceState experience;
  EntityId groupId{};
  GuestId leaderGuestId{};
  std::vector<GuestId> memberIds;
  GuestGoal currentGoal{GuestGoal::Count};
  EntityId currentTargetId{};
  double currentGoalUtility{};
};
struct Reservation : ReservationView {
  double satisfaction{70};
  double checkoutCleanliness{100};
  bool arrived{};
  EntityId groupId{};
  GuestId leaderGuestId{};
  std::vector<GuestMember> guests;
};
struct Task : TaskView {
  double total{};
  bool resourcesClaimed{};
  double trainingSkillGain{};
};
struct PendingOrder : SupplyOrderView {};
struct SavedGuestExperience {
  std::uint64_t randomState{};
  GuestExperienceState experience;
};
void writeGuestModelState(std::ostream &o, const GuestMember &guest) {
  const auto &profile = guest.profile;
  const auto &needs = guest.needs;
  o << profile.id << ' ' << guest.groupId << ' ' << guest.leaderGuestId << ' '
    << ei(guest.lifecycle) << ' ' << ei(guest.currentGoal) << ' '
    << guest.currentTargetId << ' ' << guest.currentGoalUtility << ' '
    << profile.randomState << ' ' << guest.memberIds.size();
  for (const auto id : guest.memberIds)
    o << ' ' << id;
  o << '\n' << ei(profile.archetype) << ' ' << ei(profile.ageBand) << ' '
    << ei(profile.travelPurpose) << ' ' << ei(profile.wealthBand) << ' '
    << profile.budgetPerNightCents << ' ' << profile.sensitivities.price << ' '
    << profile.sensitivities.service << ' '
    << profile.sensitivities.cleanliness << ' '
    << profile.sensitivities.noise << ' ' << profile.sensitivities.privacy
    << ' ' << profile.sensitivities.safety << ' '
    << profile.sensitivities.comfort << ' ' << profile.sensitivities.food
    << ' ' << profile.patience << ' ' << profile.socialPreference;
  for (const auto value : profile.activityPreferences)
    o << ' ' << value;
  for (const auto value : profile.roomPreferences)
    o << ' ' << value;
  for (const auto value : profile.expectations)
    o << ' ' << value;
  o << ' ' << profile.traits.size();
  for (const auto trait : profile.traits)
    o << ' ' << ei(trait);
  o << ' ' << profile.queueToleranceMultiplier << ' '
    << profile.negativeMemoryReviewWeightMultiplier << ' '
    << profile.lightSleepModifier << ' ' << profile.hygieneDecayMultiplier
    << ' ' << profile.complaintThresholdDelta << ' '
    << profile.morningPreference << ' ' << profile.eveningPreference << '\n';
  for (const auto value : needs.values)
    o << value << ' ';
  o << needs.perceptions.serviceConfidence << ' '
    << needs.perceptions.cleanlinessConfidence << ' '
    << needs.perceptions.environmentComfort << ' '
    << needs.perceptions.valuePerception << ' '
    << needs.noiseSampleElapsedSeconds << ' '
    << needs.energyPauseRemainingSeconds;
  for (const auto value : needs.recentNoiseDisruptionAgesSeconds)
    o << ' ' << value;
  o << ' ' << needs.recentNoiseDisruptionCount << '\n';
}

GuestMember readGuestModelState(std::istream &i) {
  GuestMember guest;
  auto &profile = guest.profile;
  auto &needs = guest.needs;
  int lifecycle{}, goal{};
  std::size_t memberCount{};
  i >> profile.id >> guest.groupId >> guest.leaderGuestId >> lifecycle >> goal >>
      guest.currentTargetId >> guest.currentGoalUtility >> profile.randomState >>
      memberCount;
  if (!i || profile.id == 0 || lifecycle < 0 ||
      lifecycle >= ei(GuestLifecycleState::Count) || goal < 0 ||
      goal > ei(GuestGoal::Count) || memberCount == 0 || memberCount > 4 ||
      !std::isfinite(guest.currentGoalUtility))
    throw std::invalid_argument("invalid saved guest model header");
  guest.lifecycle = static_cast<GuestLifecycleState>(lifecycle);
  guest.currentGoal = static_cast<GuestGoal>(goal);
  guest.memberIds.resize(memberCount);
  for (auto &id : guest.memberIds)
    i >> id;
  int archetype{}, age{}, purpose{}, wealth{};
  i >> archetype >> age >> purpose >> wealth >> profile.budgetPerNightCents >>
      profile.sensitivities.price >> profile.sensitivities.service >>
      profile.sensitivities.cleanliness >> profile.sensitivities.noise >>
      profile.sensitivities.privacy >> profile.sensitivities.safety >>
      profile.sensitivities.comfort >> profile.sensitivities.food >>
      profile.patience >> profile.socialPreference;
  for (auto &value : profile.activityPreferences)
    i >> value;
  for (auto &value : profile.roomPreferences)
    i >> value;
  for (auto &value : profile.expectations)
    i >> value;
  std::size_t traitCount{};
  i >> traitCount;
  if (!i || archetype < 0 || archetype >= ei(GuestArchetype::Count) || age < 0 ||
      age >= ei(GuestAgeBand::Count) || purpose < 0 ||
      purpose >= ei(GuestTravelPurpose::Count) || wealth < 0 ||
      wealth >= ei(GuestWealthBand::Count) || traitCount > 3)
    throw std::invalid_argument("invalid saved guest profile header");
  profile.archetype = static_cast<GuestArchetype>(archetype);
  profile.ageBand = static_cast<GuestAgeBand>(age);
  profile.travelPurpose = static_cast<GuestTravelPurpose>(purpose);
  profile.wealthBand = static_cast<GuestWealthBand>(wealth);
  profile.traits.resize(traitCount);
  for (auto &trait : profile.traits) {
    int value{};
    i >> value;
    if (value < 0 || value >= ei(GuestTrait::Count))
      throw std::invalid_argument("invalid saved guest trait");
    trait = static_cast<GuestTrait>(value);
  }
  i >> profile.queueToleranceMultiplier >>
      profile.negativeMemoryReviewWeightMultiplier >>
      profile.lightSleepModifier >> profile.hygieneDecayMultiplier >>
      profile.complaintThresholdDelta >> profile.morningPreference >>
      profile.eveningPreference;
  for (auto &value : needs.values)
    i >> value;
  i >> needs.perceptions.serviceConfidence >>
      needs.perceptions.cleanlinessConfidence >>
      needs.perceptions.environmentComfort >>
      needs.perceptions.valuePerception >> needs.noiseSampleElapsedSeconds >>
      needs.energyPauseRemainingSeconds;
  for (auto &value : needs.recentNoiseDisruptionAgesSeconds)
    i >> value;
  i >> needs.recentNoiseDisruptionCount;
  if (!i)
    throw std::invalid_argument("truncated saved guest model state");

  const auto inRange = [](double value, double low, double high) {
    return std::isfinite(value) && value >= low && value <= high;
  };
  const auto &s = profile.sensitivities;
  if (profile.budgetPerNightCents < 0 ||
      !inRange(s.price, 0, 1) || !inRange(s.service, 0, 1) ||
      !inRange(s.cleanliness, 0, 1) || !inRange(s.noise, 0, 1) ||
      !inRange(s.privacy, 0, 1) || !inRange(s.safety, 0, 1) ||
      !inRange(s.comfort, 0, 1) || !inRange(s.food, 0, 1) ||
      !inRange(profile.patience, 0, 1) ||
      !inRange(profile.socialPreference, 0, 1) ||
      !inRange(profile.queueToleranceMultiplier, 0, 16) ||
      !inRange(profile.negativeMemoryReviewWeightMultiplier, 0, 16) ||
      !inRange(profile.lightSleepModifier, 0, 16) ||
      !inRange(profile.hygieneDecayMultiplier, 0, 16) ||
      !inRange(profile.complaintThresholdDelta, -100, 100) ||
      !inRange(profile.morningPreference, -1, 1) ||
      !inRange(profile.eveningPreference, -1, 1) ||
      !inRange(guest.currentGoalUtility, 0, 1000000))
    throw std::invalid_argument("invalid saved guest profile values");
  for (const auto value : profile.activityPreferences)
    if (!inRange(value, 0, 1))
      throw std::invalid_argument("invalid saved guest activity preference");
  for (const auto value : profile.roomPreferences)
    if (!inRange(value, 0, 1))
      throw std::invalid_argument("invalid saved guest room preference");
  for (const auto value : profile.expectations)
    if (!inRange(value, 0, 100))
      throw std::invalid_argument("invalid saved guest expectation");
  for (const auto value : needs.values)
    if (!inRange(value, 0, 100))
      throw std::invalid_argument("invalid saved guest need");
  if (!inRange(needs.perceptions.serviceConfidence, 0, 100) ||
      !inRange(needs.perceptions.cleanlinessConfidence, 0, 100) ||
      !inRange(needs.perceptions.environmentComfort, 0, 100) ||
      !inRange(needs.perceptions.valuePerception, 0, 100) ||
      !inRange(needs.noiseSampleElapsedSeconds, 0, 1.0e9) ||
      !inRange(needs.energyPauseRemainingSeconds, 0, 300) ||
      needs.recentNoiseDisruptionCount >
          needs.recentNoiseDisruptionAgesSeconds.size())
    throw std::invalid_argument("invalid saved guest need state");
  for (const auto value : needs.recentNoiseDisruptionAgesSeconds)
    if (!inRange(value, 0, 1.0e9))
      throw std::invalid_argument("invalid saved guest noise history");
  if (std::find(guest.memberIds.begin(), guest.memberIds.end(), profile.id) ==
          guest.memberIds.end() ||
      std::find(guest.memberIds.begin(), guest.memberIds.end(),
                guest.leaderGuestId) == guest.memberIds.end())
    throw std::invalid_argument("invalid saved guest group membership");
  std::unordered_set<GuestId> uniqueMembers(guest.memberIds.begin(),
                                            guest.memberIds.end());
  if (uniqueMembers.size() != guest.memberIds.size() ||
      (guest.groupId == 0 && memberCount != 1) ||
      (guest.groupId != 0 && memberCount < 2))
    throw std::invalid_argument("invalid saved guest group");
  return guest;
}
} // namespace

struct Simulation::Impl {
  static constexpr std::size_t completedTaskHistoryLimit = 128;

  std::uint64_t seed{1};
  std::mt19937_64 rng{1};
  GuestModelDefinitions guestDefinitions{defaultGuestModelDefinitions()};
  GuestExperienceDefinitions guestExperienceDefinitions{
      defaultGuestExperienceDefinitions()};
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
  InventoryView inventory{24, 48, 36, 24, 8};
  ServiceLogisticsRuntime services{1};
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

  InventoryView serviceInventoryView() const {
    const auto &logistics = services.logistics();
    return {logistics.inventoryUsable("clean_linen_set"),
            logistics.inventoryUsable("towel_unit"),
            logistics.inventoryUsable("amenity_kit"),
            logistics.inventoryUsable("cleaning_chemical"),
            logistics.inventoryUsable("maintenance_part")};
  }

  void syncEngineeringState() {
    const auto engineering = services.engineering().snapshot();
    for (const auto &asset : engineering.assets) {
      auto *room = getRoom(asset.id);
      if (!room)
        continue;
      room->condition = asset.condition / 100.0;
      if (asset.failed && room->reservationId == 0 && !room->closed &&
          room->status != RoomStatus::Occupied &&
          room->status != RoomStatus::OutOfOrder) {
        room->status = RoomStatus::OutOfOrder;
        createTask(TaskKind::Repair, room->id, room->door, repairWork);
      }
    }
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
    for (auto &x : completedReservationHistory)
      if (x.id == id)
        return &x;
    return nullptr;
  }
  GuestMember *getGuestMember(Reservation &reservation, GuestId id) {
    for (auto &guest : reservation.guests)
      if (guest.profile.id == id)
        return &guest;
    return nullptr;
  }
  bool hasExperienceType(const GuestMember &guest,
                         GuestExperienceEventType type) const {
    return std::any_of(guest.experience.events.begin(),
                       guest.experience.events.end(),
                       [type](const GuestExperienceEvent &event) {
                         return event.type == type;
                       });
  }
  bool hasEventId(std::uint64_t eventId) const {
    if (eventId == 0)
      return false;
    const auto found = [&](const Reservation &reservation) {
      for (const auto &guest : reservation.guests)
        if (std::any_of(guest.experience.events.begin(),
                        guest.experience.events.end(),
                        [eventId](const GuestExperienceEvent &event) {
                          return event.eventId == eventId;
                        }))
          return true;
      return false;
    };
    return std::any_of(reservations.begin(), reservations.end(), found) ||
           std::any_of(completedReservationHistory.begin(),
                       completedReservationHistory.end(), found);
  }
  bool hasEntityIdInUse(EntityId id) const {
    if (id == 0)
      return true;
    for (const auto &room : rooms)
      if (room.id == id)
        return true;
    for (const auto &person : people)
      if (person.id == id)
        return true;
    for (const auto &reservation : reservations)
      if (reservation.id == id)
        return true;
    for (const auto &reservation : completedReservationHistory)
      if (reservation.id == id)
        return true;
    for (const auto &task : tasks)
      if (task.id == id)
        return true;
    for (const auto &task : completedTaskHistory)
      if (task.id == id)
        return true;
    for (const auto &order : orders)
      if (order.id == id)
        return true;
    const auto reservationContains = [id](const Reservation &reservation) {
      if (reservation.groupId == id)
        return true;
      for (const auto &guest : reservation.guests) {
        if (guest.profile.id == id || guest.groupId == id)
          return true;
        for (const auto &event : guest.experience.events)
          if (event.eventId == id || event.incidentId == id)
            return true;
        for (const auto &memory : guest.experience.memories)
          if (memory.id == id)
            return true;
        for (const auto &complaint : guest.experience.complaints)
          if (complaint.id == id)
            return true;
      }
      return false;
    };
    return std::any_of(reservations.begin(), reservations.end(),
                       reservationContains) ||
           std::any_of(completedReservationHistory.begin(),
                       completedReservationHistory.end(), reservationContains);
  }
  GuestExperienceResult applyGuestEvent(
      Reservation &reservation, GuestMember &guest,
      const GuestExperienceEvent &submitted) {
    GuestExperienceResult rejected;
    GuestExperienceEvent event = submitted;
    event.guestId = guest.profile.id;
    event.timestampSeconds = elapsed;
    if (event.locationId == 0)
      event.locationId = reservation.roomId;
    if (event.eventId != 0 &&
        (hasEventId(event.eventId) || hasEntityIdInUse(event.eventId)))
      return rejected;
    if (event.incidentId != 0 &&
        (event.incidentId == event.eventId ||
         hasEntityIdInUse(event.incidentId)))
      return rejected;

    EntityId cursor = nextId;
    constexpr auto maxId = std::numeric_limits<EntityId>::max();
    if (event.eventId > maxId - 4 || event.incidentId > maxId - 4)
      return rejected;
    if (event.eventId == 0)
      event.eventId = cursor++;
    if (event.eventId >= cursor)
      cursor = event.eventId + 1;
    if (event.incidentId != 0 && event.incidentId >= cursor)
      cursor = event.incidentId + 1;
    if (event.complaintEligible && event.incidentId == 0)
      event.incidentId = cursor++;
    const EntityId memoryId = event.rawImpact == 0.0 ? 0 : cursor++;
    const EntityId complaintId = event.complaintEligible ? cursor++ : 0;

    GuestExperienceState candidate = guest.experience;
    auto result = applyGuestExperience(guest.profile, candidate, event,
                                       guestExperienceDefinitions, memoryId,
                                       complaintId);
    if (!result.accepted)
      return result;
    guest.experience = std::move(candidate);
    reservation.satisfaction = result.overallSatisfaction;
    for (auto &person : people)
      if (person.reservation == reservation.id)
        person.satisfaction = result.overallSatisfaction;
    nextId = cursor;
    return result;
  }
  void recordReservationEvent(Reservation &reservation,
                              GuestExperienceEventType type,
                              GuestCategory category, double observed,
                              double expected, double impact,
                              bool complaintEligible,
                              std::string statement) {
    for (auto &guest : reservation.guests) {
      if (hasExperienceType(guest, type))
        continue;
      GuestExperienceEvent event;
      event.guestId = guest.profile.id;
      event.type = type;
      event.timestampSeconds = elapsed;
      event.locationId = reservation.roomId;
      event.category = category;
      event.observedValue = std::clamp(observed, 0.0, 100.0);
      event.expectedValue = std::clamp(expected, 0.0, 100.0);
      event.rawImpact = std::clamp(impact, -100.0, 100.0);
      event.salience = 0.8;
      event.complaintEligible = complaintEligible;
      event.reviewStatement = statement;
      (void)applyGuestEvent(reservation, guest, event);
    }
  }
  bool transitionGuest(GuestMember &guest, GuestLifecycleState next) {
    if (guest.lifecycle == next)
      return true;
    if (!canTransitionGuest(guest.lifecycle, next))
      return false;
    guest.lifecycle = next;
    return true;
  }
  void setGuestGoal(Reservation &reservation, GuestId id, GuestGoal goal,
                    EntityId target = 0, double utility = 0.0) {
    if (auto *guest = getGuestMember(reservation, id)) {
      guest->currentGoal = goal;
      guest->currentTargetId = target;
      guest->currentGoalUtility = utility;
    }
  }
  std::size_t groupMemberCount(const GuestProfile &leaderProfile,
                               int beds) const {
    if (beds < 2)
      return 1;
    switch (leaderProfile.archetype) {
    case GuestArchetype::CoupleLeisure:
      return 2;
    case GuestArchetype::FamilyLeisure:
    case GuestArchetype::GroupTourTraveler:
      return std::min<std::size_t>(static_cast<std::size_t>(beds), 4);
    default:
      return 1;
    }
  }
  void restoreGuestState(Reservation &reservation, bool integratedFormat) {
    if (!reservation.guests.empty())
      return;
    std::vector<const Person *> actors;
    for (const auto &person : people)
      if (person.kind == PersonKind::Guest &&
          person.reservation == reservation.id)
        actors.push_back(&person);
    std::sort(actors.begin(), actors.end(), [](const auto *a, const auto *b) {
      return a->id < b->id;
    });

    std::vector<GuestId> memberIds;
    if (!actors.empty()) {
      for (const auto *actor : actors)
        memberIds.push_back(actor->id);
    } else if (integratedFormat) {
      const GuestId leaderId = reservation.id + 1;
      const auto leaderProfile =
          generateGuestProfile(seed, leaderId, guestDefinitions);
      const Room *room = getRoom(reservation.roomId);
      const auto count = groupMemberCount(leaderProfile, room ? room->beds : 1);
      for (std::size_t index = 0; index < count; ++index)
        memberIds.push_back(leaderId + index);
    } else {
      memberIds.push_back(nextId++);
    }

    const auto *room = getRoom(reservation.roomId);
    reservation.groupId = integratedFormat && memberIds.size() > 1
                              ? reservation.id + memberIds.size() + 1
                              : 0;
    reservation.leaderGuestId = memberIds.front();
    for (std::size_t index = 0; index < memberIds.size(); ++index) {
      GuestMember member;
      member.profile =
          generateGuestProfile(seed, memberIds[index], guestDefinitions);
      if (index == 0 && memberIds.size() > 1)
        member.profile.ageBand = GuestAgeBand::Adult;
      member.groupId = reservation.groupId;
      member.leaderGuestId = reservation.leaderGuestId;
      member.memberIds = memberIds;
      const Person *actor = index < actors.size() ? actors[index] : nullptr;
      if (actor) {
        member.needs.values[static_cast<std::size_t>(GuestNeed::Energy)] =
            actor->rest;
        member.needs.values[static_cast<std::size_t>(GuestNeed::Hunger)] =
            actor->hunger;
      }
      if (reservation.completed)
        member.lifecycle = actor ? GuestLifecycleState::Departing
                                 : GuestLifecycleState::CompletedStay;
      else if (!reservation.arrived)
        member.lifecycle = GuestLifecycleState::Reserved;
      else if (!reservation.checkedIn)
        member.lifecycle = actor && actor->state == PersonState::Waiting
                               ? GuestLifecycleState::AwaitingCheckIn
                               : GuestLifecycleState::TravelingToHotel;
      else if (reservation.checkoutStarted)
        member.lifecycle = actor && actor->state == PersonState::Waiting
                               ? GuestLifecycleState::AwaitingCheckout
                               : GuestLifecycleState::PreparingCheckout;
      else if (actor && actor->goal == "Reach assigned room")
        member.lifecycle = GuestLifecycleState::CheckedIn;
      else
        member.lifecycle = GuestLifecycleState::InStay;

      if (actor && actor->goal == "Reach front desk")
        member.currentGoal = GuestGoal::ReachHotel;
      else if (actor && actor->goal == "Wait for check-in")
        member.currentGoal = GuestGoal::CheckIn;
      else if (actor && actor->goal == "Reach assigned room") {
        member.currentGoal = GuestGoal::ReachRoom;
        member.currentTargetId = reservation.roomId;
      } else if (actor && (actor->goal == "Reach front desk for checkout" ||
                           actor->goal == "Wait for checkout"))
        member.currentGoal = GuestGoal::Checkout;
      else if (actor && (actor->goal == "Leave hotel" ||
                         actor->goal == "Departed"))
        member.currentGoal = GuestGoal::LeaveHotel;
      else if (reservation.completed)
        member.currentGoal = GuestGoal::LeaveHotel;
      else if (actor && actor->state == PersonState::Sleeping)
        member.currentGoal = GuestGoal::Sleep;
      else if (reservation.arrived && reservation.checkedIn)
        member.currentGoal = GuestGoal::Relax;
      else
        member.currentGoal = GuestGoal::ReachHotel;
      if (room && member.currentGoal == GuestGoal::Relax)
        member.currentTargetId = room->id;
      reservation.guests.push_back(std::move(member));
    }
  }
  void restoreGuestStates(bool integratedFormat) {
    for (auto &reservation : reservations)
      restoreGuestState(reservation, integratedFormat);
    for (auto &reservation : completedReservationHistory)
      restoreGuestState(reservation, integratedFormat);
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
    if (kind == TaskKind::Turnover && services.requestRoomTurn(target) == 0)
      return;
    if (kind == TaskKind::Repair) {
      const auto workSeconds = static_cast<int>(std::clamp<std::int64_t>(
          static_cast<std::int64_t>(std::llround(repairWork)), 1,
          7LL * 24LL * 60LL * 60LL));
      if (services.engineering().createWorkOrder(
              target, WorkOrderType::Corrective, workSeconds) == 0)
        return;
    }
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
    std::vector<Room *> free;
    for (auto &r : rooms)
      if (!r.closed && r.status == RoomStatus::VacantReady && r.reachable)
        free.push_back(&r);
    deterministicShuffle(free, rng);
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
      if (deterministicUnitDouble(rng) > hourlyChance)
        continue;
      Reservation z;
      z.id = nextId++;
      z.guestName = "Guest " + std::to_string(z.id);
      z.roomId = r->id;
      z.arrivalDay = day + (hour > 15 ? 1 : 0);
      z.departureDay = z.arrivalDay + 1 + (int)(rng() % 3);
      z.nightlyRateCents = r->nightlyRateCents;
      z.nightlyRate = z.nightlyRateCents / 100.0;
      const GuestId leaderId = nextId++;
      auto leaderProfile =
          generateGuestProfile(seed, leaderId, guestDefinitions);
      const std::size_t memberCount = groupMemberCount(leaderProfile, r->beds);
      std::vector<GuestProfile> profiles;
      profiles.reserve(memberCount);
      profiles.push_back(std::move(leaderProfile));
      for (std::size_t member = 1; member < memberCount; ++member) {
        const GuestId guestId = nextId++;
        profiles.push_back(generateGuestProfile(seed, guestId, guestDefinitions));
      }
      z.groupId = memberCount > 1 ? nextId++ : 0;
      z.leaderGuestId = leaderId;
      std::vector<GuestId> memberIds;
      memberIds.reserve(memberCount);
      for (const auto &profile : profiles)
        memberIds.push_back(profile.id);
      if (memberCount > 1)
        profiles.front().ageBand = GuestAgeBand::Adult;
      for (auto &profile : profiles) {
        GuestMember member;
        member.profile = std::move(profile);
        member.groupId = z.groupId;
        member.leaderGuestId = leaderId;
        member.memberIds = memberIds;
        member.currentGoal = GuestGoal::ReachHotel;
        if (!transitionGuest(member, GuestLifecycleState::Reserved))
          throw std::logic_error("new guest could not enter Reserved state");
        z.guests.push_back(std::move(member));
      }
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
        for (std::size_t i = 0; i < z.guests.size(); ++i) {
          auto &member = z.guests[i];
          transitionGuest(member, GuestLifecycleState::TravelingToHotel);
          member.currentGoal = GuestGoal::ReachHotel;
          Person p;
          p.id = member.profile.id;
          p.name = i == 0 ? z.guestName : "Guest " + std::to_string(p.id);
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
          member.needs.values[static_cast<std::size_t>(GuestNeed::Energy)] =
              p.rest;
          member.needs.values[static_cast<std::size_t>(GuestNeed::Hunger)] =
              p.hunger;
          people.push_back(p);
        }
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
        for (auto &member : z.guests) {
          transitionGuest(member, GuestLifecycleState::PreparingCheckout);
          member.currentGoal = GuestGoal::Checkout;
          member.currentTargetId = 0;
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
    const double checkoutWaitMinutes = guest.queueWaitSeconds / 60.0;
    if (checkoutWaitMinutes > 5.0) {
      recordReservationEvent(
          *z, GuestExperienceEventType::LongCheckInQueue,
          GuestCategory::ArrivalDeparture, checkoutWaitMinutes, 5.0,
          -std::clamp((checkoutWaitMinutes - 5.0) * 2.0, 1.0, 45.0),
          checkoutWaitMinutes >= 15.0,
          "Checkout involved a long front desk queue.");
    }
    z->completed = true;
    economy.completedStays++;
    const auto charge = z->nightlyRateCents * (z->departureDay - z->arrivalDay);
    economy.revenueCents += charge;
    economy.cashCents += charge;
    if (auto *leader = getGuestMember(*z, z->leaderGuestId)) {
      auto review = generateGuestReview(
          leader->profile, leader->experience,
          static_cast<int>(elapsed / 86400), leader->profile.randomState,
          guestExperienceDefinitions);
      if (review.generated) {
        const auto score = static_cast<int>(std::clamp(
            std::llround(review.overallSatisfaction), 0LL, 100LL));
        reviews.push_back({z->id, review.completedDay, score, review.text,
                           review.rating, review.overallSatisfaction});
        const double multiplier = review.reputationImpactMultiplier;
        economy.reputation = std::clamp(
            economy.reputation +
                (review.overallSatisfaction - economy.reputation) * 0.15 *
                    multiplier,
            0.0, 100.0);
      }
    }
    guest.destination = entrance();
    guest.state = PersonState::Traveling;
    guest.goal = "Leave hotel";
    for (auto &member : z->guests) {
      transitionGuest(member, GuestLifecycleState::Departing);
      member.currentGoal = GuestGoal::LeaveHotel;
      member.currentTargetId = 0;
    }
    for (auto &person : people)
      if (person.reservation == z->id && person.id != guest.id) {
        person.destination = entrance();
        person.state = PersonState::Traveling;
        person.goal = "Leave hotel";
      }
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
    return (task.kind == TaskKind::Turnover || task.kind == TaskKind::Restock)
               ? p.kind == PersonKind::Housekeeper
           : task.kind == TaskKind::Repair ? p.kind == PersonKind::Maintenance
                                           : p.kind == PersonKind::Receptionist;
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
    std::vector<EntityId> repairedRooms;
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
          if (!owner || !owner->onShift) {
            if (owner)
              owner->breakTaskCreated = false;
            task.employeeId = 0;
            task.status = TaskStatus::Completed;
            continue;
          }
        }

        bool resources = true;
        if (task.kind == TaskKind::Turnover) {
          const auto &logistics = services.logistics();
          resources = task.resourcesClaimed ||
                      (has(TileKind::SupplyCloset) &&
                       logistics.inventoryUsable("clean_linen_set") >= 1 &&
                       logistics.inventoryUsable("towel_unit") >= 2 &&
                       logistics.inventoryUsable("amenity_kit") >= 1 &&
                       logistics.inventoryUsable("cleaning_chemical") >= 1);
        }
        if (task.kind == TaskKind::Repair)
          resources = task.resourcesClaimed ||
                      services.logistics().inventoryUsable("maintenance_part") >= 1;
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
        int dist = std::numeric_limits<int>::max();
        for (auto &p : people)
          if (p.onShift && !p.absent && p.task == 0 &&
              p.goal != "Preventive maintenance" && eligible(p, task)) {
            const int d = manhattan(p.position, task.target);
            if (d < dist || (d == dist && (!best || p.id < best->id))) {
              dist = d;
              best = &p;
            }
          }
        if (!best)
          continue;

        task.employeeId = best->id;
        best->task = task.id;
        best->destination =
            (task.kind == TaskKind::Turnover && !task.resourcesClaimed)
                ? supply()
                : task.target;
        best->state = PersonState::Traveling;
        task.status = TaskStatus::Traveling;
        if (task.kind == TaskKind::Repair && !task.resourcesClaimed)
          task.resourcesClaimed = true;
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
            const auto &logistics = services.logistics();
            if (logistics.inventoryUsable("clean_linen_set") < 1 ||
                logistics.inventoryUsable("towel_unit") < 2 ||
                logistics.inventoryUsable("amenity_kit") < 1 ||
                logistics.inventoryUsable("cleaning_chemical") < 1) {
              task.status = TaskStatus::Blocked;
              task.blockedReason = "Required physical supplies unavailable";
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
                ? std::clamp(100.0 * (1.0 - task.workRemainingSeconds / task.total),
                             0.0, 100.0)
                : 100.0;
      } else {
        p->onBreak = false;
        p->inTraining = false;
        if (task.kind == TaskKind::Turnover) {
          const auto serviceWork = services.workRoomTurnSecond(task.targetId);
          if (!serviceWork.valid || serviceWork.blockedReason != BlockReason::None) {
            task.status = TaskStatus::Blocked;
            task.blockedReason = !serviceWork.valid
                                     ? "FINAL-04 room-turn job missing"
                                     : "FINAL-04 room-turn resources blocked";
            task.employeeId = 0;
            p->task = 0;
            p->state = PersonState::Idle;
            continue;
          }
        } else if (task.kind == TaskKind::Repair) {
          const auto serviceWork = services.workEngineeringSecond(
              task.targetId, WorkOrderType::Corrective);
          if (!serviceWork.valid || serviceWork.blockedReason != BlockReason::None) {
            task.status = TaskStatus::Blocked;
            task.blockedReason = !serviceWork.valid
                                     ? "FINAL-04 engineering work order missing"
                                     : "FINAL-04 engineering resources blocked";
            task.employeeId = 0;
            p->task = 0;
            p->state = PersonState::Idle;
            continue;
          }
        }
        const double fatiguePerHour =
            task.kind == TaskKind::Turnover
                ? 10.0
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

      const bool serviceTurnComplete =
          task.kind != TaskKind::Turnover ||
          services.housekeeping().roomStatus(task.targetId) ==
              ServiceRoomStatus::Ready;
      bool serviceRepairComplete = task.kind != TaskKind::Repair;
      if (task.kind == TaskKind::Repair) {
        const auto engineering = services.engineering().snapshot();
        for (const auto &order : engineering.workOrders)
          if (order.assetId == task.targetId &&
              order.type == WorkOrderType::Corrective &&
              order.stage == WorkOrderStage::Completed)
            serviceRepairComplete = true;
      }
      if (task.workRemainingSeconds > 0 || !serviceTurnComplete ||
          !serviceRepairComplete)
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
        p->skill = std::clamp(p->skill + task.trainingSkillGain, 0.0, 100.0);
        continue;
      }
      if (task.kind == TaskKind::CheckIn) {
        if (auto *guest = getPerson(task.targetId)) {
          if (auto *reservation = getReservation(guest->reservation)) {
            if (auto *room = getRoom(reservation->roomId)) {
              reservation->checkedIn = true;
              room->status = RoomStatus::Occupied;
              for (auto &member : reservation->guests) {
                if (member.lifecycle == GuestLifecycleState::TravelingToHotel)
                  transitionGuest(member, GuestLifecycleState::Arriving);
                if (member.lifecycle == GuestLifecycleState::Arriving)
                  transitionGuest(member, GuestLifecycleState::AwaitingCheckIn);
                transitionGuest(member, GuestLifecycleState::CheckedIn);
                member.currentGoal = GuestGoal::ReachRoom;
                member.currentTargetId = room->id;
              }
              for (auto &person : people)
                if (person.reservation == reservation->id) {
                  person.destination = room->door;
                  person.state = PersonState::Traveling;
                  person.goal = "Reach assigned room";
                }
              const double queueMinutes = guest->queueWaitSeconds / 60.0;
              if (queueMinutes > 5.0) {
                recordReservationEvent(
                    *reservation, GuestExperienceEventType::LongCheckInQueue,
                    GuestCategory::ArrivalDeparture, queueMinutes, 5.0,
                    -std::clamp((queueMinutes - 5.0) * 2.0, 1.0, 45.0),
                    queueMinutes >= 15.0,
                    "Check-in involved a long front desk queue.");
              } else {
                recordReservationEvent(
                    *reservation, GuestExperienceEventType::FastCheckIn,
                    GuestCategory::ArrivalDeparture, queueMinutes, 5.0,
                    std::clamp((5.0 - queueMinutes) * 2.0, 1.0, 10.0), false,
                    "The front desk checked us in quickly.");
              }
              if (room->cleanliness >= 85.0) {
                recordReservationEvent(
                    *reservation,
                    GuestExperienceEventType::ExcellentRoomCleanliness,
                    GuestCategory::Cleanliness, room->cleanliness, 70.0,
                    std::clamp((room->cleanliness - 70.0) * 0.35, 1.0, 12.0),
                    false, "Our room was exceptionally clean.");
              }
              for (auto &member : reservation->guests) {
                GuestPerceptionEvidence evidence;
                evidence.cleanlinessConfidence = room->cleanliness;
                member.needs.perceptions = updateGuestPerceptions(
                    member.needs.perceptions, evidence);
              }
            }
          }
        }
      }
      if (task.kind == TaskKind::CheckOut)
        if (auto *guest = getPerson(task.targetId))
          completeCheckout(*guest);
      if (auto *room = getRoom(task.targetId)) {
        if (task.kind == TaskKind::Turnover && !room->closed) {
          if (room->condition < 35) {
            room->status = RoomStatus::OutOfOrder;
            failedRooms.push_back(room->id);
          } else {
            room->status = RoomStatus::VacantReady;
            room->cleanliness = std::clamp(
                70 + p->skill * 0.3 - p->fatigue * 0.1, 0.0, 100.0);
          }
        }
        if (task.kind == TaskKind::Repair) {
          const auto engineering = services.engineering().snapshot();
          for (const auto &asset : engineering.assets)
            if (asset.id == room->id)
              room->condition = asset.condition / 100.0;
          if (!room->closed) {
            room->status = RoomStatus::VacantDirty;
            repairedRooms.push_back(room->id);
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

    // FINAL-03 scheduler supplies labor to authoritative FINAL-04 preventive work.
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
      if (!serviceWork.valid || serviceWork.blockedReason != BlockReason::None) {
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
        auto *reservation = getReservation(p.reservation);
        auto *member = reservation ? getGuestMember(*reservation, p.id) : nullptr;
        if (p.state == PersonState::Sleeping && hour >= 7 && hour < 22) {
          p.state = PersonState::Idle;
          p.goal = "Relax in room";
        } else if (p.state == PersonState::Idle && (hour >= 22 || hour < 7)) {
          p.state = PersonState::Sleeping;
          p.goal = "Sleep in room";
        }
        if (member && member->lifecycle == GuestLifecycleState::InStay)
          setGuestGoal(*reservation, p.id,
                       p.state == PersonState::Sleeping ? GuestGoal::Sleep
                                                        : GuestGoal::Relax,
                       reservation->roomId);
        if (member) {
          const GuestActivity activity =
              p.state == PersonState::Sleeping
                  ? GuestActivity::Sleeping
              : p.state == PersonState::Idle ? GuestActivity::Relaxing
                                             : GuestActivity::Awake;
          member->needs = updateGuestNeeds(mem…13812 tokens truncated…0, 50.0);
  event.salience = 0.9;
  event.complaintEligible = update.complaintEligible;
  event.reviewStatement = "Noise interrupted our sleep.";
  const auto result = impl_->applyGuestEvent(*reservation, *guest, event);
  if (!result.accepted)
    return {false, "Sleep disturbance could not be recorded"};
  return {true, "Sleep disturbance recorded", result.memoryId};
}

CommandResult Simulation::resolveGuestComplaint(
    GuestId guestId, EntityId complaintId, GuestRecoveryOption option) {
  if (guestId == 0 || complaintId == 0)
    return {false, "Guest and complaint identities are required"};
  Reservation *reservation = nullptr;
  GuestMember *guest = nullptr;
  for (auto &candidate : impl_->reservations) {
    if (auto *member = impl_->getGuestMember(candidate, guestId)) {
      reservation = &candidate;
      guest = member;
      break;
    }
  }
  if (!reservation || !guest || reservation->completed)
    return {false, "Guest is not in an active stay"};
  auto complaint = std::find_if(
      guest->experience.complaints.begin(), guest->experience.complaints.end(),
      [complaintId](const GuestComplaint &candidate) {
        return candidate.id == complaintId && candidate.open;
      });
  if (complaint == guest->experience.complaints.end())
    return {false, "Open guest complaint was not found"};
  auto memory = std::find_if(
      guest->experience.memories.begin(), guest->experience.memories.end(),
      [&](const GuestMemory &candidate) {
        return candidate.id == complaint->memoryId && !candidate.resolved &&
               candidate.incidentId == complaint->incidentId;
      });
  if (memory == guest->experience.memories.end())
    return {false, "Complaint no longer has an unresolved memory"};
  if (option == GuestRecoveryOption::FreeDrink ||
      option == GuestRecoveryOption::RoomUpgrade)
    return {false, "Required food or room service is not available"};

  GuestRecoveryContext context;
  context.staffHospitality = 0.75;
  for (const auto &person : impl_->people)
    if (person.kind == PersonKind::Receptionist)
      context.staffHospitality = std::clamp(person.skill / 100.0, 0.3, 1.0);
  context.guestForgiveness =
      std::find(guest->profile.traits.begin(), guest->profile.traits.end(),
                GuestTrait::Forgiving) != guest->profile.traits.end()
          ? 1.0
          : 0.8;
  if (impl_->elapsed > memory->timestampSeconds)
    context.responseMinutes =
        (impl_->elapsed - memory->timestampSeconds) / 60.0;
  context.complaintUrgency = complaint->urgency;
  context.issueFixed = false;
  context.superiorRoomAvailable = false;
  const auto recovery = calculateGuestRecovery(
      *memory, option, context, impl_->guestExperienceDefinitions);
  if (!recovery)
    return {false, "Recovery option is unavailable or invalid"};

  GuestExperienceEvent event;
  event.guestId = guestId;
  event.type = GuestExperienceEventType::Recovery;
  event.timestampSeconds = impl_->elapsed;
  event.locationId = reservation->roomId;
  event.category = memory->category;
  event.observedValue =
      std::clamp(100.0 - recovery->remainingNegativeMagnitude, 0.0, 100.0);
  event.expectedValue = 70.0;
  event.rawImpact = std::max(0.1, recovery->magnitudeReduction);
  event.salience = 0.8;
  event.resolved = true;
  event.resolvesIncidentId = complaint->incidentId;
  event.resolvedMagnitudeReduction = recovery->magnitudeReduction;
  event.reviewStatement = "Staff responded to our complaint.";
  const auto applied = impl_->applyGuestEvent(*reservation, *guest, event);
  if (!applied.accepted || !applied.complaintResolved)
    return {false, "Guest recovery could not be recorded"};

  std::int64_t refundCents = 0;
  if (option == GuestRecoveryOption::PartialRoomRefund)
    refundCents = reservation->nightlyRateCents / 2;
  else if (option == GuestRecoveryOption::FullNightRefund)
    refundCents = reservation->nightlyRateCents;
  impl_->economy.cashCents -= refundCents;
  impl_->economy.revenueCents -= refundCents;
  return {true, "Guest complaint resolved", complaintId};
}

void Simulation::step(double seconds) {
  if (!std::isfinite(seconds) || seconds < 0 || seconds > 1.0e9)
    throw std::invalid_argument("step seconds must be finite and bounded");
  impl_->remainderMillis += (std::int64_t)std::llround(seconds * 1000);
  while (impl_->remainderMillis >= 1000) {
    impl_->remainderMillis -= 1000;
    impl_->minute();
    impl_->services.tickSecond();
    impl_->syncEngineeringState();
  }
}

LogisticsSnapshot Simulation::logisticsSnapshot() const {
  return impl_->services.logisticsSnapshot();
}
TaskId Simulation::requestRoomTurn(RoomId roomId) {
  auto *room = impl_->getRoom(roomId);
  if (!room || room->reservationId != 0 || room->status == RoomStatus::Occupied ||
      room->status == RoomStatus::OutOfOrder || room->condition < 40)
    return 0;
  const auto serviceTask = impl_->services.requestRoomTurn(roomId);
  if (serviceTask == 0)
    return 0;
  room->status = RoomStatus::VacantDirty;
  impl_->createTask(TaskKind::Turnover, roomId, room->door,
                    impl_->turnoverWork);
  return serviceTask;
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
  const auto appendGuestViews = [&](const Reservation &reservation) {
    for (const auto &member : reservation.guests) {
      GuestView guest;
      guest.profile = member.profile;
      guest.needs = member.needs;
      guest.experience = member.experience;
      guest.activeMemories.reserve(member.experience.memories.size());
      for (const auto &memory : member.experience.memories)
        if (!memory.resolved)
          guest.activeMemories.push_back(memory);
      std::sort(guest.activeMemories.begin(), guest.activeMemories.end(),
                [](const auto &a, const auto &b) {
                  if (a.timestampSeconds != b.timestampSeconds)
                    return a.timestampSeconds < b.timestampSeconds;
                  return a.id < b.id;
                });
      guest.reservationId = reservation.id;
      guest.groupId = member.groupId;
      guest.leaderGuestId = member.leaderGuestId;
      guest.memberIds = member.memberIds;
      guest.lifecycle = member.lifecycle;
      guest.currentGoal = member.currentGoal;
      guest.currentTargetId = member.currentTargetId;
      guest.currentGoalUtility = member.currentGoalUtility;
      guest.queueToleranceMinutes = guestQueueToleranceMinutes(
          60.0, member.profile, 1.0, 1.0);
      guest.reviewScoreNoiseRange =
          impl_->guestExperienceDefinitions.reviewScoreNoiseRange;
      switch (member.currentGoal) {
      case GuestGoal::ReachHotel:
      case GuestGoal::CheckIn:
      case GuestGoal::ReachRoom:
      case GuestGoal::Checkout:
      case GuestGoal::LeaveHotel: {
        const auto selection = selectGuestGoal(
            member.profile, member.needs, std::span<const GuestGoalCandidate>{},
            member.currentGoal);
        if (selection)
          guest.goalSelection = *selection;
        break;
      }
      case GuestGoal::Sleep:
      case GuestGoal::Relax: {
        GuestGoalCandidate candidate;
        candidate.goal = member.currentGoal;
        candidate.targetId = member.currentTargetId;
        candidate.requiredNeed = member.currentGoal == GuestGoal::Sleep
                                     ? GuestNeed::Energy
                                     : GuestNeed::Comfort;
        candidate.preference = 1.0;
        candidate.queueToleranceMinutes = guest.queueToleranceMinutes;
        const auto selection = selectGuestGoal(
            member.profile, member.needs,
            std::span<const GuestGoalCandidate>(&candidate, 1), std::nullopt);
        if (selection)
          guest.goalSelection = *selection;
        break;
      }
      default:
        break;
      }
      v.guests.push_back(std::move(guest));
    }
  };
  for (const auto &r : impl_->reservations)
    appendGuestViews(r);
  for (const auto &r : impl_->completedReservationHistory)
    appendGuestViews(r);
  std::sort(v.guests.begin(), v.guests.end(), [](const auto &a, const auto &b) {
    return a.profile.id < b.profile.id;
  });
  for (auto &t : impl_->tasks)
    v.tasks.push_back(t);
  for (auto &t : impl_->completedTaskHistory)
    v.tasks.push_back(t);
  v.reviews = impl_->reviews;
  v.inventory = impl_->serviceInventoryView();
  v.economy = impl_->economy;
  for (auto &o : impl_->orders)
    v.supplyOrders.push_back(o);
  v.departments = departments();
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
  inv(impl_->serviceInventoryView());
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
      << ' ' << p.reliability << ' ' << p.contract.sourceApplicantId << ' '
      << ei(p.contract.role) << ' ' << p.contract.hourlyWageCents << ' '
      << p.contract.onboardingCostCents << ' ' << p.contract.shiftStartHour << ' '
      << p.contract.shiftEndHour << ' ' << p.morale << ' ' << p.absent << ' '
      << p.onBreak << ' ' << p.inTraining << ' ' << p.breakMinutesTakenToday
      << ' ' << p.trainingProgress << ' ' << p.shiftWorkedSeconds << ' '
      << p.shiftInstanceKey << ' ' << p.breakTaskCreated << '\n';
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
      << t.resourcesClaimed << ' ' << t.notBeforeSecond << ' '
      << t.trainingSkillGain << '\n';
  for (auto &t : impl_->completedTaskHistory)
    o << t.id << ' ' << ei(t.kind) << ' ' << ei(t.status) << ' ' << t.targetId
      << ' ' << t.employeeId << ' ' << t.target.floor << ' ' << t.target.x
      << ' ' << t.target.y << ' ' << t.workRemainingSeconds << ' '
      << std::quoted(t.blockedReason) << ' ' << t.total << ' '
      << t.resourcesClaimed << ' ' << t.notBeforeSecond << ' '
      << t.trainingSkillGain << '\n';
  o << impl_->reviews.size() << '\n';
  for (auto &r : impl_->reviews)
    o << r.reservationId << ' ' << r.day << ' ' << r.score << ' ' << r.rating
      << ' ' << r.overallSatisfaction << ' ' << std::quoted(r.text) << '\n';
  o << impl_->orders.size() << '\n';
  for (auto &p : impl_->orders) {
    o << p.id << ' ';
    inv(p.items);
    o << ' ' << p.etaDay << ' ' << p.delivered << '\n';
  }
  o << impl_->onboardingCostCents << ' ' << impl_->staffBreakAfterMinutes << ' '
    << impl_->staffBreakDurationMinutes << ' ' << impl_->missedBreakFatiguePerHour
    << ' ' << impl_->missedBreakMoralePerHour << ' ' << impl_->trainingSkillGain
    << ' ' << impl_->consumedApplicantDay << ' '
    << impl_->consumedApplicantIds.size();
  for (const auto applicantId : impl_->consumedApplicantIds)
    o << ' ' << applicantId;
  o << ' ' << impl_->managers.size();
  for (const auto &manager : impl_->managers)
    o << ' ' << ei(manager.department) << ' ' << manager.managerId;
  o << '\n';
  const auto serviceState = impl_->services.save();
  o << "FINAL04 " << serviceState.size() << '\n';
  o.write(serviceState.data(), static_cast<std::streamsize>(serviceState.size()));
  o << '\n';
  std::size_t guestCount = 0;
  for (const auto &reservation : impl_->reservations)
    guestCount += reservation.guests.size();
  for (const auto &reservation : impl_->completedReservationHistory)
    guestCount += reservation.guests.size();
  std::ostringstream guestPayload;
  guestPayload << std::setprecision(17) << guestCount << '\n';
  const auto writeGuest = [&](const GuestMember &guest) {
    writeGuestModelState(guestPayload, guest);
    guestPayload << guest.experience.isGroupLeader;
    for (const auto value : guest.experience.categoryStartingSatisfaction)
      guestPayload << ' ' << value;
    for (const auto value : guest.experience.categorySatisfaction)
      guestPayload << ' ' << value;
    guestPayload << ' ' << guest.experience.events.size() << '\n';
    for (const auto &event : guest.experience.events)
      guestPayload << event.eventId << ' ' << event.guestId << ' '
        << event.incidentId
        << ' ' << ei(event.type) << ' ' << event.timestampSeconds << ' '
        << event.locationId << ' ' << event.sourceEntityId.has_value() << ' '
        << event.sourceEntityId.value_or(0) << ' ' << ei(event.category) << ' '
        << event.observedValue << ' ' << event.expectedValue << ' '
        << event.rawImpact << ' ' << event.salience << ' '
        << event.complaintEligible << ' ' << event.critical << ' '
        << event.resolved << ' ' << event.resolvesIncidentId.has_value() << ' '
        << event.resolvesIncidentId.value_or(0) << ' '
        << event.resolvedMagnitudeReduction << ' '
        << std::quoted(event.reviewStatement) << '\n';
    guestPayload << guest.experience.memories.size() << '\n';
    for (const auto &memory : guest.experience.memories)
      guestPayload << memory.id << ' ' << memory.eventId << ' '
        << memory.incidentId
        << ' ' << ei(memory.type) << ' ' << memory.timestampSeconds << ' '
        << memory.locationId << ' ' << memory.sourceEntityId.has_value() << ' '
        << memory.sourceEntityId.value_or(0) << ' ' << ei(memory.category) << ' '
        << memory.valence << ' ' << memory.magnitude << ' ' << memory.salience
        << ' ' << memory.decayHalfLifeHours << ' ' << memory.critical << ' '
        << memory.resolved << ' ' << std::quoted(memory.reviewStatement)
        << '\n';
    guestPayload << guest.experience.complaints.size() << '\n';
    for (const auto &complaint : guest.experience.complaints)
      guestPayload << complaint.id << ' ' << complaint.incidentId << ' '
        << complaint.memoryId << ' ' << ei(complaint.urgency) << ' '
        << complaint.open << '\n';
  };
  for (const auto &reservation : impl_->reservations)
    for (const auto &guest : reservation.guests)
      writeGuest(guest);
  for (const auto &reservation : impl_->completedReservationHistory)
    for (const auto &guest : reservation.guests)
      writeGuest(guest);
  const auto guestBytes = guestPayload.str();
  o << "GUEST10 1 " << guestBytes.size() << '\n';
  o.write(guestBytes.data(), static_cast<std::streamsize>(guestBytes.size()));
  o << '\n';
  return o.str();
}
Simulation Simulation::load(std::string_view data) {
  if (data.size() > 64 * 1024 * 1024)
    throw std::invalid_argument("simulation save too large");
  std::istringstream i{std::string(data)};
  std::string magic;
  int version, w, h, f;
  bool integratedGuestFormat = false;
  bool fullGuestFormat = false;
  std::unordered_map<GuestId, SavedGuestExperience> savedGuestExperience;
  std::unordered_map<GuestId, GuestMember> savedGuestStates;
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
    i >> p.reservation >> p.task;
    if (version >= 6)
      i >> p.accruedWageUnits;
    else
      i >> legacyAccruedWageMicros;
    int contractRole = ei(StaffRole::Housekeeper);
    if (version >= 8)
      i >> p.reliability >> p.contract.sourceApplicantId >> contractRole >>
          p.contract.hourlyWageCents >> p.contract.onboardingCostCents >>
          p.contract.shiftStartHour >> p.contract.shiftEndHour >> p.morale >>
          p.absent >> p.onBreak >> p.inTraining >> p.breakMinutesTakenToday >>
          p.trainingProgress >> p.shiftWorkedSeconds >> p.shiftInstanceKey >>
          p.breakTaskCreated;
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
        !std::isfinite(p.reliability) || p.reliability < 0 ||
        p.reliability > 100 || contractRole < ei(StaffRole::Receptionist) ||
        contractRole > ei(StaffRole::Maintenance) ||
        p.contract.hourlyWageCents < 0 ||
        p.contract.hourlyWageCents > 1000000 ||
        p.contract.onboardingCostCents < 0 ||
        p.contract.onboardingCostCents > 100000000 ||
        p.contract.shiftStartHour < 0 || p.contract.shiftStartHour > 23 ||
        p.contract.shiftEndHour < 0 || p.contract.shiftEndHour > 23 ||
        !std::isfinite(p.morale) || p.morale < 0 || p.morale > 100 ||
        p.breakMinutesTakenToday < 0 || p.breakMinutesTakenToday > 24 * 60 ||
        !std::isfinite(p.trainingProgress) || p.trainingProgress < 0 ||
        p.trainingProgress > 100 || p.shiftWorkedSeconds < 0 ||
        p.shiftWorkedSeconds > 48 * 3600)
      throw std::invalid_argument("invalid saved person");
    p.kind = static_cast<PersonKind>(k);
    p.state = static_cast<PersonState>(st);
    if (version >= 8)
      p.contract.role = static_cast<StaffRole>(contractRole);
    else if (p.kind != PersonKind::Guest) {
      p.reliability = 100.0;
      p.morale = 100.0;
      p.absent = false;
      p.onBreak = false;
      p.inTraining = false;
      p.breakMinutesTakenToday = 0;
      p.trainingProgress = 0;
      p.shiftWorkedSeconds = 0;
      p.shiftInstanceKey = std::numeric_limits<std::int64_t>::min();
      p.breakTaskCreated = false;
      p.contract = {0, staffRole(p.kind), p.hourlyWageCents, 0,
                    p.shiftStartHour, p.shiftEndHour};
    }
    if (p.kind != PersonKind::Guest &&
        (personKind(p.contract.role) != p.kind ||
         p.contract.hourlyWageCents != p.hourlyWageCents ||
         p.contract.shiftStartHour != p.shiftStartHour ||
         p.contract.shiftEndHour != p.shiftEndHour))
      throw std::invalid_argument("invalid saved employee contract");
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
    if (version >= 8)
      i >> t.notBeforeSecond >> t.trainingSkillGain;
    if (k < ei(TaskKind::CheckIn) || k > ei(TaskKind::Training) ||
        st < ei(TaskStatus::Ready) || st > ei(TaskStatus::Completed) ||
        !d.inside(t.target) || !std::isfinite(t.workRemainingSeconds) ||
        !std::isfinite(t.total) || t.notBeforeSecond < 0 ||
        !std::isfinite(t.trainingSkillGain) || t.trainingSkillGain < 0 ||
        t.trainingSkillGain > 100)
      throw std::invalid_argument("invalid saved task");
    t.kind = static_cast<TaskKind>(k);
    t.status = static_cast<TaskStatus>(st);
  }
  i >> n;
  if (n > 100000)
    throw std::invalid_argument("too many saved reviews");
  d.reviews.resize(n);
  for (auto &r : d.reviews) {
    i >> r.reservationId >> r.day >> r.score;
    if (version >= 10)
      i >> r.rating >> r.overallSatisfaction;
    i >> std::quoted(r.text);
    if (version < 10) {
      r.rating = std::clamp(r.score / 10.0, 1.0, 10.0);
      r.overallSatisfaction = r.score;
    }
  }
  for (const auto &r : d.reviews)
    if (r.day < 0 || r.score < 0 || r.score > 100 ||
        !std::isfinite(r.rating) || r.rating < 1 || r.rating > 10 ||
        !std::isfinite(r.overallSatisfaction) ||
        r.overallSatisfaction < 0 || r.overallSatisfaction > 100)
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
    std::size_t applicantCount{};
    i >> d.onboardingCostCents >> d.staffBreakAfterMinutes >>
        d.staffBreakDurationMinutes >> d.missedBreakFatiguePerHour >>
        d.missedBreakMoralePerHour >> d.trainingSkillGain >>
        d.consumedApplicantDay >> applicantCount;
    if (d.onboardingCostCents < 0 || d.onboardingCostCents > 100000000 ||
        d.staffBreakAfterMinutes < 0 || d.staffBreakAfterMinutes > 100000 ||
        d.staffBreakDurationMinutes <= 0 ||
        d.staffBreakDurationMinutes > 24 * 60 ||
        !std::isfinite(d.missedBreakFatiguePerHour) ||
        d.missedBreakFatiguePerHour < 0 || d.missedBreakFatiguePerHour > 1000 ||
        !std::isfinite(d.missedBreakMoralePerHour) ||
        d.missedBreakMoralePerHour < 0 || d.missedBreakMoralePerHour > 1000 ||
        !std::isfinite(d.trainingSkillGain) || d.trainingSkillGain < 0 ||
        d.trainingSkillGain > 100 || d.consumedApplicantDay < -1 ||
        applicantCount > 1000)
      throw std::invalid_argument("invalid saved workforce settings");
    d.consumedApplicantIds.resize(applicantCount);
    std::unordered_set<ApplicantId> applicantIds;
    for (auto &applicantId : d.consumedApplicantIds) {
      i >> applicantId;
      if (applicantId == 0 || !applicantIds.insert(applicantId).second)
        throw std::invalid_argument("invalid saved applicant IDs");
    }
    std::size_t managerCount{};
    i >> managerCount;
    if (managerCount > allDepartments().size())
      throw std::invalid_argument("invalid saved manager count");
    d.managers.resize(managerCount);
    std::unordered_set<int> managedDepartments;
    for (auto &manager : d.managers) {
      int department{};
      i >> department >> manager.managerId;
      if (department < ei(DepartmentId::FrontOffice) ||
          department > ei(DepartmentId::Engineering) || manager.managerId == 0 ||
          !managedDepartments.insert(department).second)
        throw std::invalid_argument("invalid saved manager assignment");
      manager.department = static_cast<DepartmentId>(department);
    }
  }
  if (version >= 9) {
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
    if (!d.services.setScenarioInventory(
            d.inventory.linen, d.inventory.towels, d.inventory.amenities,
            d.inventory.chemicals, d.inventory.parts))
      throw std::invalid_argument("legacy inventory exceeds FINAL-04 capacity");
    for (const auto &room : d.rooms) {
      d.services.registerRoom(
          room.id, room.status == RoomStatus::VacantReady
                       ? ServiceRoomStatus::Ready
                       : ServiceRoomStatus::Blocked);
      d.services.registerAsset(
          room.id,
          std::clamp(static_cast<int>(std::llround(room.condition * 100.0)),
                     0, 10000));
    }
  }
  int guestVersion{};
  bool hasGuestExtension = false;
  if (version == 10) {
    std::string guestTag;
    std::size_t guestBytes{};
    i >> guestTag >> guestVersion >> guestBytes;
    if (!i || guestTag != "GUEST10" || guestVersion != 1 ||
        guestBytes > 48 * 1024 * 1024 || i.get() != '\n')
      throw std::invalid_argument("invalid GUEST-10 save section");
    std::string payload(guestBytes, '\0');
    i.read(payload.data(), static_cast<std::streamsize>(guestBytes));
    if (!i || static_cast<std::size_t>(i.gcount()) != guestBytes ||
        i.get() != '\n')
      throw std::invalid_argument("truncated GUEST-10 save section");
    i >> std::ws;
    if (!i.eof())
      throw std::invalid_argument("unexpected trailing guest save data");
    i.clear();
    i.str(std::move(payload));
    i.seekg(0);
    fullGuestFormat = true;
    hasGuestExtension = true;
  } else if (version >= 9 &&
             i.peek() != std::char_traits<char>::eof()) {
    std::string guestTag;
    i >> guestTag >> guestVersion;
    if (!i || guestTag != "GUEST9" || guestVersion < 1 || guestVersion > 2)
      throw std::invalid_argument("invalid guest save extension");
    hasGuestExtension = true;
  }
  if (version >= 9 && hasGuestExtension) {
    integratedGuestFormat = true;
    if (fullGuestFormat || guestVersion == 2) {
        std::size_t guestCount{};
        i >> guestCount;
        if (!i || guestCount > 100000)
          throw std::invalid_argument("too many saved guest states");
        std::size_t totalEvents{}, totalMemories{}, totalComplaints{};
        const auto readSource = [&](bool present, EntityId value,
                                    std::optional<EntityId> &destination) {
          if (present) {
            if (value == 0)
              return false;
            destination = value;
          } else if (value != 0) {
            return false;
          }
          return true;
        };
        for (std::size_t guestIndex = 0; guestIndex < guestCount;
             ++guestIndex) {
          GuestId guestId{};
          SavedGuestExperience saved;
          GuestMember savedModel;
          bool isGroupLeader{};
          if (fullGuestFormat) {
            savedModel = readGuestModelState(i);
            guestId = savedModel.profile.id;
            saved.randomState = savedModel.profile.randomState;
            i >> isGroupLeader;
          } else {
            i >> guestId >> saved.randomState >> isGroupLeader;
          }
          saved.experience.isGroupLeader = isGroupLeader;
          for (auto &value : saved.experience.categoryStartingSatisfaction)
            i >> value;
          for (auto &value : saved.experience.categorySatisfaction)
            i >> value;
          std::size_t eventCount{};
          i >> eventCount;
          if (!i || guestId == 0 || eventCount > 100000 ||
              totalEvents + eventCount > 100000)
            throw std::invalid_argument("invalid saved guest state header");
          for (const auto value :
               saved.experience.categoryStartingSatisfaction)
            if (!std::isfinite(value) || value < 0 || value > 100)
              throw std::invalid_argument("invalid saved guest baseline");
          for (const auto value : saved.experience.categorySatisfaction)
            if (!std::isfinite(value) || value < 0 || value > 100)
              throw std::invalid_argument("invalid saved guest satisfaction");
          totalEvents += eventCount;
          saved.experience.events.resize(eventCount);
          for (auto &event : saved.experience.events) {
            int type{}, category{};
            bool hasSource{}, hasResolution{};
            EntityId source{}, resolves{};
            i >> event.eventId >> event.guestId >> event.incidentId >> type >>
                event.timestampSeconds >> event.locationId >> hasSource >>
                source >> category >> event.observedValue >>
                event.expectedValue >> event.rawImpact >> event.salience >>
                event.complaintEligible >> event.critical >> event.resolved >>
                hasResolution >> resolves >> event.resolvedMagnitudeReduction >>
                std::quoted(event.reviewStatement);
            if (!i || type < 0 || type >= ei(GuestExperienceEventType::Count) ||
                category < 0 || category >= ei(GuestCategory::Count) ||
                event.eventId == 0 || event.guestId != guestId ||
                event.timestampSeconds < 0 || event.locationId == 0 ||
                !readSource(hasSource, source, event.sourceEntityId) ||
                !readSource(hasResolution, resolves,
                            event.resolvesIncidentId) ||
                !std::isfinite(event.observedValue) ||
                event.observedValue < 0 || event.observedValue > 100 ||
                !std::isfinite(event.expectedValue) ||
                event.expectedValue < 0 || event.expectedValue > 100 ||
                !std::isfinite(event.rawImpact) || event.rawImpact < -100 ||
                event.rawImpact > 100 || !std::isfinite(event.salience) ||
                event.salience < 0 || event.salience > 1 ||
                !std::isfinite(event.resolvedMagnitudeReduction) ||
                event.resolvedMagnitudeReduction < 0 ||
                event.resolvedMagnitudeReduction > 100 ||
                event.reviewStatement.size() > 512 ||
                (event.complaintEligible && event.incidentId == 0))
              throw std::invalid_argument("invalid saved guest event");
            event.type = static_cast<GuestExperienceEventType>(type);
            event.category = static_cast<GuestCategory>(category);
          }

          std::size_t memoryCount{};
          i >> memoryCount;
          if (!i || memoryCount > 100000 ||
              totalMemories + memoryCount > 100000)
            throw std::invalid_argument("too many saved guest memories");
          totalMemories += memoryCount;
          saved.experience.memories.resize(memoryCount);
          for (auto &memory : saved.experience.memories) {
            int type{}, category{};
            bool hasSource{};
            EntityId source{};
            i >> memory.id >> memory.eventId >> memory.incidentId >> type >>
                memory.timestampSeconds >> memory.locationId >> hasSource >>
                source >> category >> memory.valence >> memory.magnitude >>
                memory.salience >> memory.decayHalfLifeHours >> memory.critical >>
                memory.resolved >> std::quoted(memory.reviewStatement);
            if (!i || type < 0 || type >= ei(GuestExperienceEventType::Count) ||
                category < 0 || category >= ei(GuestCategory::Count) ||
                memory.id == 0 || memory.eventId == 0 ||
                memory.timestampSeconds < 0 || memory.locationId == 0 ||
                !readSource(hasSource, source, memory.sourceEntityId) ||
                !std::isfinite(memory.valence) || memory.valence < -1 ||
                memory.valence > 1 || !std::isfinite(memory.magnitude) ||
                memory.magnitude < 0 || memory.magnitude > 100 ||
                !std::isfinite(memory.salience) || memory.salience < 0 ||
                memory.salience > 1 ||
                !std::isfinite(memory.decayHalfLifeHours) ||
                memory.decayHalfLifeHours < 0.001 ||
                memory.decayHalfLifeHours > 87600 ||
                memory.reviewStatement.size() > 512)
              throw std::invalid_argument("invalid saved guest memory");
            memory.type = static_cast<GuestExperienceEventType>(type);
            memory.category = static_cast<GuestCategory>(category);
          }

          std::size_t complaintCount{};
          i >> complaintCount;
          if (!i || complaintCount > 100000 ||
              totalComplaints + complaintCount > 100000)
            throw std::invalid_argument("too many saved guest complaints");
          totalComplaints += complaintCount;
          saved.experience.complaints.resize(complaintCount);
          for (auto &complaint : saved.experience.complaints) {
            int urgency{};
            i >> complaint.id >> complaint.incidentId >> complaint.memoryId >>
                urgency >> complaint.open;
            if (!i || complaint.id == 0 || complaint.incidentId == 0 ||
                complaint.memoryId == 0 || urgency < 0 ||
                urgency >= ei(GuestComplaintUrgency::Count))
              throw std::invalid_argument("invalid saved guest complaint");
            complaint.urgency =
                static_cast<GuestComplaintUrgency>(urgency);
          }
          if (fullGuestFormat) {
            savedModel.experience = std::move(saved.experience);
            if (!savedGuestStates.emplace(guestId, std::move(savedModel)).second)
              throw std::invalid_argument("duplicate saved guest identity");
          } else if (!savedGuestExperience.emplace(guestId, std::move(saved)).second) {
            throw std::invalid_argument("duplicate saved guest identity");
          }
        }
  }
  }
  d.services.engineering().setConditionLossPerDayHundredths(
      static_cast<int>(std::llround(d.roomConditionLossPerDay * 100.0)));
  d.inventory = d.serviceInventoryView();
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
  if (fullGuestFormat) {
    std::unordered_map<EntityId, EntityId> groupReservations;
    const auto validateGuestGroups = [&](const std::vector<Reservation> &items) {
      for (const auto &reservation : items) {
        std::vector<GuestId> reservationIds;
        for (const auto &guest : reservation.guests)
          reservationIds.push_back(guest.profile.id);
        auto sortedIds = reservationIds;
        std::sort(sortedIds.begin(), sortedIds.end());
        for (const auto &guest : reservation.guests) {
          if (guest.profile.id >= d.nextId || guest.leaderGuestId == 0 ||
              guest.leaderGuestId >= d.nextId || guest.groupId >= d.nextId)
            throw std::invalid_argument("saved guest identity is out of range");
          auto members = guest.memberIds;
          std::sort(members.begin(), members.end());
          if (members != sortedIds)
            throw std::invalid_argument("saved guest member reference is broken");
          if (guest.groupId != 0) {
            const auto [group, inserted] =
                groupReservations.emplace(guest.groupId, reservation.id);
            if ((inserted && entityIds.contains(guest.groupId)) ||
                (!inserted && group->second != reservation.id))
              throw std::invalid_argument(
                  "saved guest group identity is duplicated");
            if (inserted)
              entityIds.insert(guest.groupId);
          }
        }
      }
    };
    validateGuestGroups(d.reservations);
    validateGuestGroups(d.completedReservationHistory);
  }
  for (const auto &manager : d.managers) {
    const auto person = personById.find(manager.managerId);
    if (person == personById.end() ||
        person->second->kind == PersonKind::Guest ||
        staffRole(person->second->kind) != departmentRole(manager.department))
      throw std::invalid_argument("invalid saved manager reference");
  }

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
    const auto guestCount = guestsByReservation[reservation.id];
    const auto maxGuestCount =
        room == roomById.end()
            ? std::size_t{4}
            : std::min<std::size_t>(static_cast<std::size_t>(room->second->beds),
                                    4);
    const bool guestCountValid =
        reservation.completed
            ? guestCount <= maxGuestCount
            : reservation.arrived
                  ? guestCount > 0 && guestCount <= maxGuestCount
                  : guestCount == 0;
    if ((!reservation.completed && room == roomById.end()) ||
        (reservation.checkedIn && !reservation.arrived) ||
        (reservation.checkoutStarted && !reservation.checkedIn) ||
        (reservation.completed && !reservation.checkoutStarted) ||
        !guestCountValid)
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
  d.restoreGuestStates(integratedGuestFormat);
  if (fullGuestFormat) {
    std::size_t restoredGuestCount{};
    const auto restoreModels = [&](std::vector<Reservation> &reservations) {
      for (auto &reservation : reservations) {
        std::vector<GuestId> reservationMembers;
        reservationMembers.reserve(reservation.guests.size());
        for (const auto &guest : reservation.guests)
          reservationMembers.push_back(guest.profile.id);
        const auto canonicalMembers = reservationMembers;
        std::sort(reservationMembers.begin(), reservationMembers.end());
        if (std::adjacent_find(reservationMembers.begin(),
                              reservationMembers.end()) !=
            reservationMembers.end())
          throw std::invalid_argument("duplicate guest in reservation");
        for (auto &guest : reservation.guests) {
          const auto found = savedGuestStates.find(guest.profile.id);
          if (found == savedGuestStates.end())
            throw std::invalid_argument("saved guest state has an unknown identity");
          guest = found->second;
          auto expectedMembers = guest.memberIds;
          std::sort(expectedMembers.begin(), expectedMembers.end());
          if (expectedMembers != reservationMembers ||
              guest.leaderGuestId == 0 ||
              (guest.groupId == 0 && guest.memberIds.size() != 1) ||
              (guest.groupId != 0 && guest.memberIds.size() < 2))
            throw std::invalid_argument("saved guest group reference is invalid");
          ++restoredGuestCount;
        }
        if (!reservation.guests.empty()) {
          const auto &leader = reservation.guests.front();
          for (const auto &guest : reservation.guests)
            if (guest.groupId != leader.groupId ||
                guest.leaderGuestId != leader.leaderGuestId ||
                guest.memberIds != leader.memberIds)
              throw std::invalid_argument("saved reservation group is inconsistent");
          if (std::find(canonicalMembers.begin(), canonicalMembers.end(),
                        leader.leaderGuestId) == canonicalMembers.end())
            throw std::invalid_argument("saved group leader is not a member");
          reservation.groupId = leader.groupId;
          reservation.leaderGuestId = leader.leaderGuestId;
        }
      }
    };
    restoreModels(d.reservations);
    restoreModels(d.completedReservationHistory);
    if (restoredGuestCount != savedGuestStates.size())
      throw std::invalid_argument("saved guest state has an unknown identity");
  } else if (!savedGuestExperience.empty()) {
    std::size_t restoredGuestCount{};
    const auto restoreExperiences = [&](std::vector<Reservation> &reservations) {
      for (auto &reservation : reservations)
        for (auto &guest : reservation.guests) {
          const auto found = savedGuestExperience.find(guest.profile.id);
          if (found == savedGuestExperience.end())
            continue;
          guest.profile.randomState = found->second.randomState;
          guest.experience = found->second.experience;
          ++restoredGuestCount;
        }
    };
    restoreExperiences(d.reservations);
    restoreExperiences(d.completedReservationHistory);
    if (restoredGuestCount != savedGuestExperience.size())
      throw std::invalid_argument("saved guest state has an unknown identity");
  }
  if (fullGuestFormat) {
    std::unordered_set<EntityId> extensionIds = entityIds;
    const auto claimExtensionId = [&](EntityId id) {
      if (id == 0 || id >= d.nextId || !extensionIds.insert(id).second)
        throw std::invalid_argument("duplicate or invalid guest entity reference");
    };
    std::unordered_set<GuestId> guestIds;
    std::size_t guestRecordCount{}, memoryCount{}, complaintCount{};
    const auto validateExperience = [&](const std::vector<Reservation> &items) {
      for (const auto &reservation : items) {
        for (const auto &guest : reservation.guests) {
          ++guestRecordCount;
          if (!guestIds.insert(guest.profile.id).second)
            throw std::invalid_argument("duplicate saved guest identity");
          const auto person = personById.find(guest.profile.id);
          if (person != personById.end() &&
              (person->second->kind != PersonKind::Guest ||
               person->second->reservation != reservation.id))
            throw std::invalid_argument("saved guest actor reference is broken");

          std::unordered_map<EntityId, const GuestExperienceEvent *> events;
          std::unordered_set<EntityId> incidents;
          for (const auto &event : guest.experience.events) {
            claimExtensionId(event.eventId);
            if (!events.emplace(event.eventId, &event).second)
              throw std::invalid_argument("duplicate guest event reference");
            if (event.incidentId != 0) {
              claimExtensionId(event.incidentId);
              incidents.insert(event.incidentId);
            }
            if (!roomById.contains(event.locationId) ||
                (event.sourceEntityId &&
                 !personById.contains(*event.sourceEntityId)))
              throw std::invalid_argument("guest event location or source is unknown");
          }
          for (const auto &event : guest.experience.events)
            if (event.resolvesIncidentId &&
                !incidents.contains(*event.resolvesIncidentId))
              throw std::invalid_argument("guest resolution references an unknown incident");

          std::unordered_map<EntityId, const GuestMemory *> memories;
          for (const auto &memory : guest.experience.memories) {
            claimExtensionId(memory.id);
            if (!memories.emplace(memory.id, &memory).second ||
                !events.contains(memory.eventId) ||
                !roomById.contains(memory.locationId) ||
                (memory.sourceEntityId &&
                 !personById.contains(*memory.sourceEntityId)) ||
                (memory.incidentId != 0 &&
                 !incidents.contains(memory.incidentId)))
              throw std::invalid_argument("guest memory references are broken");
          }
          for (const auto &complaint : guest.experience.complaints) {
            claimExtensionId(complaint.id);
            ++complaintCount;
            const auto memory = memories.find(complaint.memoryId);
            if (memory == memories.end() ||
                memory->second->incidentId != complaint.incidentId)
              throw std::invalid_argument("guest complaint references are broken");
          }
          memoryCount += guest.experience.memories.size();
        }
      }
    };
    validateExperience(d.reservations);
    validateExperience(d.completedReservationHistory);
    if (guestRecordCount > 100000 || memoryCount > 100000 ||
        complaintCount > 100000 || guestRecordCount != savedGuestStates.size())
      throw std::invalid_argument("saved guest collections exceed their limit");
  }
  d.compactTransientState();
  return s;
}

} // namespace hh::game

