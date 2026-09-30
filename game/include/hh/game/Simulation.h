#pragma once

#include "hh/game/ConstructionTypes.h"
#include "hh/game/ServiceLogistics.h"
#include "hh/game/Departments.h"
#include "hh/game/GuestModel.h"
#include "hh/game/GuestExperience.h"
#include "hh/game/StaffOptimization.h"
#include "hh/game/Workforce.h"
#include "hh/game/ServiceTypes.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace hh::game {

enum class RoomStatus {
  Incomplete,
  VacantReady,
  Reserved,
  Occupied,
  VacantDirty,
  Cleaning,
  OutOfOrder
};
enum class PersonKind { Guest, Receptionist, Housekeeper, Maintenance };
enum class PersonState {
  OffDuty,
  Idle,
  Traveling,
  Working,
  Waiting,
  Sleeping,
  CheckedOut
};
enum class TaskKind {
  CheckIn,
  Turnover,
  Restock,
  Repair,
  CheckOut,
  Break,
  Training
};
enum class TaskStatus { Ready, Traveling, Working, Blocked, Completed };

struct TileView {
  Position position;
  TileKind kind{TileKind::Empty};
};
struct RoomView {
  EntityId id{};
  std::string name;
  Position door;
  RoomStatus status{RoomStatus::Incomplete};
  int floor{};
  int x{};
  int y{};
  int width{};
  int height{};
  int area{};
  int capacity{};
  std::vector<Position> tiles;
  std::optional<GridEdge> primaryDoorEdge;
  std::vector<std::string> diagnostics;
  int beds{};
  int baths{};
  double cleanliness{};
  double condition{};
  std::int64_t nightlyRateCents{};
  double nightlyRate{};
  EntityId reservationId{};
  bool reachable{};
  bool closed{};
};
struct ConstructionObjectView {
  EntityId id{};
  ConstructionObjectKind kind{};
  Position position;
  int quarterTurns{};
  std::vector<Position> footprint;
};
struct PersonView {
  EntityId id{};
  std::string name;
  PersonKind kind{PersonKind::Guest};
  PersonState state{PersonState::Idle};
  Position position;
  Position destination;
  double fatigue{};
  double satisfaction{};
  double hunger{};
  double rest{};
  double patience{};
  double skill{};
  std::int64_t hourlyWageCents{};
  int shiftStartHour{};
  int shiftEndHour{};
  std::int64_t travelSeconds{};
  int queueWaitSeconds{};
  std::string goal;
  bool onShift{};
  double reliability{100.0};
  double morale{100.0};
  bool absent{};
  bool onBreak{};
  bool inTraining{};
  int breakMinutesTakenToday{};
  double trainingProgress{};
  EmployeeContract contract;
};
struct ReservationView {
  EntityId id{};
  std::string guestName;
  EntityId roomId{};
  int arrivalDay{};
  int departureDay{};
  std::int64_t nightlyRateCents{};
  double nightlyRate{};
  bool checkedIn{};
  bool checkoutStarted{};
  bool completed{};
};
struct GuestView {
  GuestProfile profile;
  GuestNeedState needs;
  GuestExperienceState experience;
  std::optional<GuestGoalSelection> goalSelection;
  std::vector<GuestMemory> activeMemories;
  EntityId reservationId{};
  EntityId groupId{};
  GuestId leaderGuestId{};
  std::vector<GuestId> memberIds;
  GuestLifecycleState lifecycle{GuestLifecycleState::Prospective};
  GuestGoal currentGoal{GuestGoal::Count};
  EntityId currentTargetId{};
  double currentGoalUtility{};
  double queueToleranceMinutes{60.0};
  double reviewRatingMinimum{1.0};
  double reviewRatingMaximum{10.0};
  double reviewScoreNoiseRange{};
  std::optional<std::int64_t> marketReferenceNightlyRateCents;
  std::optional<double> measuredRoomNoise;
};
struct TaskView {
  EntityId id{};
  TaskKind kind{TaskKind::Turnover};
  TaskStatus status{TaskStatus::Ready};
  EntityId targetId{};
  EntityId employeeId{};
  Position target;
  double workRemainingSeconds{};
  std::string blockedReason;
  std::int64_t notBeforeSecond{};
};
struct ReviewView {
  EntityId reservationId{};
  int day{};
  int score{};
  std::string text;
  double rating{1.0};
  double overallSatisfaction{};
};
struct InventoryView {
  int linen{};
  int towels{};
  int amenities{};
  int chemicals{};
  int parts{};
};
struct SupplyOrderView {
  EntityId id{};
  InventoryView items;
  int etaDay{};
  bool delivered{};
};
struct EconomyView {
  std::int64_t cashCents{};
  std::int64_t revenueCents{};
  std::int64_t payrollCents{};
  std::int64_t supplyCostCents{};
  std::int64_t constructionCostCents{};
  std::int64_t utilityCostCents{};
  double occupancy{};
  double reputation{};
  int completedStays{};
  int stars{};
  bool distressed{};
};
struct SimulationView {
  std::int64_t elapsedSeconds{};
  int day{};
  int hour{};
  int width{};
  int height{};
  int floors{};
  std::vector<TileView> tiles;
  std::vector<GridEdge> constructionWalls;
  std::vector<GridEdge> constructionDoors;
  std::vector<ConstructionObjectView> constructionObjects;
  std::vector<RoomView> rooms;
  std::vector<PersonView> people;
  std::vector<ReservationView> reservations;
  std::vector<GuestView> guests;
  std::vector<TaskView> tasks;
  std::vector<ReviewView> reviews;
  InventoryView inventory;
  EconomyView economy;
  std::vector<SupplyOrderView> supplyOrders;
  std::vector<DepartmentView> departments;
};

struct RoomBlueprint {
  std::string name;
  int floor{};
  int x{};
  int y{};
  int width{};
  int height{};
  Position door;
  int beds{1};
  int baths{1};
  double nightlyRate{120.0};
};
struct StaffHire {
  std::string name;
  PersonKind role{PersonKind::Housekeeper};
  int shiftStartHour{6};
  int shiftEndHour{14};
  double hourlyWage{18.0};
};
struct SupplyOrder {
  int linen{};
  int towels{};
  int amenities{};
  int chemicals{};
  int parts{};
};
struct CommandResult {
  bool ok{};
  std::string message;
  EntityId id{};
  explicit operator bool() const noexcept { return ok; }
};

class Simulation {
public:
  explicit Simulation(std::uint64_t seed = 1, int width = 32, int height = 20,
                      int floors = 1);
  ~Simulation();
  Simulation(Simulation &&) noexcept;
  Simulation &operator=(Simulation &&) noexcept;
  Simulation(const Simulation &);
  Simulation &operator=(const Simulation &);

  static Simulation tutorial(std::uint64_t seed = 1);
  CommandResult buildTile(Position, TileKind);
  CommandResult setConstructionWall(GridEdge edge, bool enabled);
  CommandResult setConstructionDoor(GridEdge edge, bool enabled);
  CommandResult removeConstructionEdge(GridEdge edge);
  CommandResult placeConstructionObject(ConstructionObjectKind kind,
                                        Position anchor,
                                        int quarterTurns = 0);
  CommandResult removeConstructionObject(EntityId objectId);
  CommandResult buildFurnishedRoom(const RoomBlueprint &);
  CommandResult hireStaff(const StaffHire &);
  [[nodiscard]] std::vector<Applicant> applicants() const;
  HireResult hireApplicant(ApplicantId applicantId);
  CommandResult fireStaff(EntityId employeeId);
  CommandResult setStaffShift(EntityId employeeId, int startHour, int endHour);
  CommandResult scheduleTraining(EntityId employeeId,
                                 std::int64_t startSecond,
                                 int durationMinutes = 60);
  CommandResult assignDepartmentManager(DepartmentId department,
                                        EntityId employeeId);
  [[nodiscard]] std::vector<DepartmentView> departments() const;
  [[nodiscard]] DepartmentForecast departmentForecast(DepartmentId department,
                                                      SimDay day) const;
  [[nodiscard]] OptimizerSnapshot buildOptimizerSnapshot() const;
  [[nodiscard]] PlanValidation validatePlan(const OptimizerSnapshot &snapshot,
                                            const AssignmentPlan &plan) const;
  CommandResult setRoomRate(EntityId roomId, double rate);
  CommandResult requestClean(EntityId roomId);
  CommandResult requestRepair(EntityId roomId);
  CommandResult closeRoom(EntityId roomId, bool closed);
  CommandResult removeRoom(EntityId roomId);
  CommandResult orderSupplies(const SupplyOrder &);
  CommandResult loadDefinitions(std::string_view jsonText);
  CommandResult reportGuestExperience(const GuestExperienceEvent &event);
  CommandResult reportGuestSleepNoise(GuestId guestId,
                                      std::optional<double> measuredNoiseDb);
  CommandResult resolveGuestComplaint(GuestId guestId, EntityId complaintId,
                                      GuestRecoveryOption option);

  [[nodiscard]] LogisticsSnapshot logisticsSnapshot() const;
  [[nodiscard]] TaskId requestRoomTurn(RoomId roomId);
  [[nodiscard]] LaundryBatchId requestLaundryBatch(int quantity);
  [[nodiscard]] WorkOrderId createWorkOrder(AssetId assetId, WorkOrderType type);
  [[nodiscard]] RoomServiceOrderId placeRoomServiceOrder(
      GuestId guestId, const RoomServiceOrder &order);
  [[nodiscard]] bool markRoomServiceProductionReady(RoomServiceOrderId orderId);
  [[nodiscard]] bool requestRoomServiceTrayPickup(RoomServiceOrderId orderId);

  void step(double seconds);

  [[nodiscard]] SimulationView view() const;
  [[nodiscard]] bool isReachable(Position from, Position to) const;
  [[nodiscard]] std::string save() const;
  static Simulation load(std::string_view data);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace hh::game

