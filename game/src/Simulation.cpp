#include "hh/game/Simulation.h"
#include "hh/assets/Json.h"
#include <algorithm>
#include <array>
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
  std::vector<ManagerAssignment> managers;
  InventoryView inventory{24, 48, 36, 24, 8};
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
  bool transientStateDirty{};
  std::unordered_map<EntityId, std::size_t> roomIndexById;
  std::unordered_map<EntityId, std::size_t> personIndexById;
  std::unordered_map<EntityId, std::size_t> reservationIndexById;
  std::vector<std::size_t> staffIndices;
  std::array<std::vector<std::size_t>, 3> staffByRole;
  std::vector<std::size_t> guestIndices;
  std::array<std::vector<std::size_t>, 3> readyTaskIndices;
  bool entityIndicesDirty{true};
  int staffScheduleHour{-1};

  void markEntityIndicesDirty() noexcept { entityIndicesDirty = true; }
  void rebuildEntityIndices() {
    roomIndexById.clear();
    personIndexById.clear();
    reservationIndexById.clear();
    staffIndices.clear();
    guestIndices.clear();
    for (auto &role : staffByRole)
      role.clear();

    roomIndexById.reserve(rooms.size());
    personIndexById.reserve(people.size());
    reservationIndexById.reserve(reservations.size());
    staffIndices.reserve(people.size());
    guestIndices.reserve(people.size());
    for (std::size_t index = 0; index < rooms.size(); ++index)
      roomIndexById.emplace(rooms[index].id, index);
    for (std::size_t index = 0; index < people.size(); ++index) {
      personIndexById.emplace(people[index].id, index);
      if (people[index].kind == PersonKind::Guest) {
        guestIndices.push_back(index);
      } else {
        staffIndices.push_back(index);
        staffByRole[static_cast<std::size_t>(
                        ei(staffRole(people[index].kind)))]
            .push_back(index);
      }
    }
    for (std::size_t index = 0; index < reservations.size(); ++index)
      reservationIndexById.emplace(reservations[index].id, index);
    entityIndicesDirty = false;
  }
  void ensureEntityIndices() {
    if (entityIndicesDirty)
      rebuildEntityIndices();
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
  template <typename Visitor>
  void forEachNeighbor(Position p, Visitor &&visit) const {
    const auto tryNeighbor = [&](Position q) {
      if (passable(q) &&
          (q.floor == p.floor || map[index(q)] == TileKind::Stairs))
        visit(q);
    };
    tryNeighbor({p.floor, p.x + 1, p.y});
    tryNeighbor({p.floor, p.x - 1, p.y});
    tryNeighbor({p.floor, p.x, p.y + 1});
    tryNeighbor({p.floor, p.x, p.y - 1});
    if (inside(p) && map[index(p)] == TileKind::Stairs) {
      tryNeighbor({p.floor + 1, p.x, p.y});
      tryNeighbor({p.floor - 1, p.x, p.y});
    }
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
      forEachNeighbor(p, [&](Position n) {
        if (prev[index(n)] < 0) {
          prev[index(n)] = index(p);
          q.push(n);
        }
      });
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
    ensureEntityIndices();
    const auto found = roomIndexById.find(id);
    return found == roomIndexById.end() ? nullptr : &rooms[found->second];
  }
  Person *getPerson(EntityId id) {
    ensureEntityIndices();
    const auto found = personIndexById.find(id);
    return found == personIndexById.end() ? nullptr : &people[found->second];
  }
  Reservation *getReservation(EntityId id) {
    ensureEntityIndices();
    const auto found = reservationIndexById.find(id);
    return found == reservationIndexById.end()
               ? nullptr
               : &reservations[found->second];
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
    std::vector<Room *> free;
    for (auto &r : rooms)
      if (!r.closed && r.status == RoomStatus::VacantReady && r.reachable)
        free.push_back(&r);
    std::shuffle(free.begin(), free.end(), rng);
    const double reputationUtility =
        0.2 + 0.8 * std::clamp((economy.reputation - 60.0) / 20.0, 0.0, 1.0);
    bool addedReservation = false;
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
      addedReservation = true;
      r->status = RoomStatus::Reserved;
      r->reservationId = z.id;
    }
    if (addedReservation)
      markEntityIndicesDirty();
  }
  void arrivals(int day) {
    if (!has(TileKind::FrontDesk))
      return;
    bool addedGuest = false;
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
        addedGuest = true;
      }
    if (addedGuest)
      markEntityIndicesDirty();
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
    markTransientStateDirty();
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
  void refreshStaffSchedule(int day, int hour) {
    ensureEntityIndices();
    for (const auto personIndex : staffIndices) {
      auto &p = people[personIndex];
      if (!shiftActive(p, hour)) {
        p.onShift = false;
        p.absent = false;
        p.onBreak = false;
        p.inTraining = false;
        p.state = PersonState::OffDuty;
        p.task = 0;
        continue;
      }

      const auto key = shiftInstanceKey(p, day, hour);
      if (p.shiftInstanceKey != key) {
        for (auto &task : tasks)
          if (task.kind == TaskKind::Break && task.targetId == p.id &&
              task.status != TaskStatus::Completed) {
            task.employeeId = 0;
            task.status = TaskStatus::Completed;
            markTransientStateDirty();
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
      }
    }
    staffScheduleHour = hour;
  }
  void postAccruedWage(Person &person) {
    const std::int64_t cents = person.accruedWageUnits / 3600;
    person.accruedWageUnits %= 3600;
    economy.payrollCents += cents;
    economy.cashCents -= cents;
  }
  void markTransientStateDirty() noexcept { transientStateDirty = true; }
  void staffAndTasks() {
    const int hour = static_cast<int>((elapsed / 3600) % 24);
    const int day = static_cast<int>(elapsed / 86400);
    ensureEntityIndices();
    if (staffScheduleHour != hour)
      refreshStaffSchedule(day, hour);
    std::vector<EntityId> repairedRooms;
    std::vector<EntityId> failedRooms;

    const int breakDueSeconds = staffBreakAfterMinutes * 60;
    for (const auto personIndex : staffIndices) {
      auto &p = people[personIndex];
      if (p.onShift) {
        p.accruedWageUnits += p.hourlyWageCents;
        if (p.state == PersonState::OffDuty)
          p.state = PersonState::Idle;
        if (!p.onBreak && !p.inTraining)
          ++p.shiftWorkedSeconds;

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
      } else if (p.absent) {
        p.onShift = false;
        p.onBreak = false;
        p.inTraining = false;
        p.state = PersonState::OffDuty;
        p.task = 0;
      } else {
        p.onShift = false;
        p.onBreak = false;
        p.inTraining = false;
        p.state = PersonState::OffDuty;
        p.fatigue = std::max(0.0, p.fatigue - 12.0 / 3600.0);
        p.task = 0;
      }
    }

    for (auto &readyTasks : readyTaskIndices)
      readyTasks.clear();
    for (std::size_t taskIndex = 0; taskIndex < tasks.size(); ++taskIndex) {
      const auto &task = tasks[taskIndex];
      if (elapsed < task.notBeforeSecond ||
          (task.status != TaskStatus::Ready &&
           task.status != TaskStatus::Blocked))
        continue;
      const int priority = task.kind == TaskKind::Break
                               ? 0
                               : task.kind == TaskKind::Training ? 1 : 2;
      readyTaskIndices[static_cast<std::size_t>(priority)].push_back(taskIndex);
    }

    auto assignReady = [&](const std::vector<std::size_t> &readyTasks) {
      for (const auto taskIndex : readyTasks) {
        auto &task = tasks[taskIndex];
        if (elapsed < task.notBeforeSecond ||
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
            markTransientStateDirty();
            continue;
          }
        }

        if (task.kind == TaskKind::Turnover) {
          const auto *room = getRoom(task.targetId);
          if (room && room->closed) {
            task.status = TaskStatus::Blocked;
            task.blockedReason = "Room is closed";
            continue;
          }
        }

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
        if (!resources) {
          task.status = TaskStatus::Blocked;
          task.blockedReason = "Required local supplies unavailable";
          continue;
        }
        task.blockedReason.clear();
        task.status = TaskStatus::Ready;

        Person *best = nullptr;
        int dist = std::numeric_limits<int>::max();
        if (task.kind == TaskKind::Break || task.kind == TaskKind::Training) {
          auto *owner = getPerson(task.targetId);
          if (owner && owner->onShift && owner->task == 0)
            best = owner;
        } else {
          const auto role = task.kind == TaskKind::Turnover ||
                                    task.kind == TaskKind::Restock
                                ? StaffRole::Housekeeper
                                : task.kind == TaskKind::Repair
                                      ? StaffRole::Maintenance
                                      : StaffRole::Receptionist;
          for (const auto personIndex :
               staffByRole[static_cast<std::size_t>(ei(role))]) {
            auto &p = people[personIndex];
            if (p.onShift && p.task == 0) {
              const int d = manhattan(p.position, task.target);
              if (d < dist || (d == dist && (!best || p.id < best->id))) {
                dist = d;
                best = &p;
              }
            }
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
        if (task.kind == TaskKind::Repair && !task.resourcesClaimed) {
          inventory.parts--;
          task.resourcesClaimed = true;
        }
        if (task.kind == TaskKind::Turnover)
          if (auto *room = getRoom(task.targetId))
            room->status = RoomStatus::Cleaning;
      }
    };

    for (const auto &readyTasks : readyTaskIndices)
      assignReady(readyTasks);

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
          markTransientStateDirty();
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
            if (inventory.linen < 1 || inventory.towels < 2 ||
                inventory.amenities < 1 || inventory.chemicals < 1) {
              task.status = TaskStatus::Blocked;
              task.blockedReason = "Required local supplies unavailable";
              task.employeeId = 0;
              p->task = 0;
              p->state = PersonState::Idle;
              continue;
            }
            inventory.linen--;
            inventory.towels -= 2;
            inventory.amenities--;
            inventory.chemicals--;
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

      if (task.workRemainingSeconds > 0)
        continue;

      task.status = TaskStatus::Completed;
      markTransientStateDirty();
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
              guest->destination = room->door;
              guest->state = PersonState::Traveling;
              guest->goal = "Reach assigned room";
              reservation->checkedIn = true;
              room->status = RoomStatus::Occupied;
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
          room->condition = 100;
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
  }
  void guests() {
    const int hour = static_cast<int>((elapsed / 3600) % 24);
    ensureEntityIndices();
    for (const auto personIndex : guestIndices) {
      auto &p = people[personIndex];
      if (p.state != PersonState::CheckedOut) {
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
              markTransientStateDirty();
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
        if (auto *reservation = getReservation(p.reservation))
          reservation->satisfaction = p.satisfaction;
      }
    }
  }
  void compactTransientState() {
    if (!transientStateDirty)
      return;
    const auto peopleEnd = std::remove_if(
        people.begin(), people.end(), [](const auto &p) {
          return p.kind == PersonKind::Guest &&
                 p.state == PersonState::CheckedOut;
        });
    const bool peopleChanged = peopleEnd != people.end();
    people.erase(peopleEnd, people.end());

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
    const auto reservationsEnd = std::remove_if(
        reservations.begin(), reservations.end(),
        [](const auto &reservation) { return reservation.completed; });
    const bool reservationsChanged = reservationsEnd != reservations.end();
    reservations.erase(reservationsEnd, reservations.end());
    if (peopleChanged || reservationsChanged)
      markEntityIndicesDirty();
    transientStateDirty = false;
    ensureEntityIndices();
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
      if (!r.closed && r.status != RoomStatus::Incomplete &&
          r.status != RoomStatus::OutOfOrder) {
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
  impl_->markEntityIndicesDirty();
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
  p.reliability = 100.0;
  p.contract = {0, staffRole(h.role), p.hourlyWageCents, 0,
                h.shiftStartHour, h.shiftEndHour};
  impl_->people.push_back(p);
  impl_->markEntityIndicesDirty();
  impl_->staffScheduleHour = -1;
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
  impl_->markEntityIndicesDirty();
  impl_->staffScheduleHour = -1;
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
  impl_->markEntityIndicesDirty();
  impl_->staffScheduleHour = -1;
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
  impl_->staffScheduleHour = -1;
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
    if (kind == TaskKind::Repair)
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
  impl_->markEntityIndicesDirty();
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
      d.hungerRate < 0 || d.restLoss < 0 ||
      d.roomConditionLossPerDay < 0 || d.roomConditionLossPerDay > 100 ||
      d.staffBreakDurationMinutes <= 0 || d.staffBreakDurationMinutes > 24 * 60 ||
      d.missedBreakFatiguePerHour < 0 || d.missedBreakFatiguePerHour > 1000 ||
      d.missedBreakMoralePerHour < 0 || d.missedBreakMoralePerHour > 1000 ||
      d.trainingSkillGain < 0 || d.trainingSkillGain > 100)
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
  v.departments = departments();
  return v;
}

std::string Simulation::save() const {
  std::ostringstream o;
  o << std::setprecision(17) << "HHGS 8 " << impl_->seed << ' ' << impl_->width
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
    o << r.reservationId << ' ' << r.day << ' ' << r.score << ' '
      << std::quoted(r.text) << '\n';
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
  return o.str();
}
Simulation Simulation::load(std::string_view data) {
  if (data.size() > 64 * 1024 * 1024)
    throw std::invalid_argument("simulation save too large");
  std::istringstream i{std::string(data)};
  std::string magic;
  int version, w, h, f;
  i >> magic >> version;
  if (magic != "HHGS" || version < 2 || version > 8)
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
  d.transientStateDirty = true;
  d.compactTransientState();
  d.markEntityIndicesDirty();
  d.rebuildEntityIndices();
  d.staffScheduleHour = -1;
  return s;
}

} // namespace hh::game
