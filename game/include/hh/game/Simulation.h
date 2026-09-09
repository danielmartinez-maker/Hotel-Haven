#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace hh::game {

using EntityId = std::uint64_t;

struct ConstructionCommand;
struct ConstructionPreview;
struct ConstructionResult;
struct ConstructionMaterials;
struct BuildPlan;
struct BuildQueueResult;
struct ConstructionSnapshot;
enum class UtilityKind;
enum class InfrastructureKind;
struct BuildingSystemsSnapshot;
struct RoomSaleValidation;
struct ElevatorSpec;
struct GuestPsychologySnapshot;
struct GuestOpportunitySnapshot;
struct GoalSelection;
struct SatisfactionBreakdown;
struct ExperienceEvent;

struct Position {
  int floor{};
  int x{};
  int y{};
  bool operator==(const Position &) const = default;
};
enum class TileKind {
  Empty,
  Floor,
  Wall,
  Door,
  Entrance,
  FrontDesk,
  SupplyCloset,
  Stairs,
  Bathroom,
  StaffRoom,
  Lobby
};
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
enum class GuestArchetype {
  BudgetLeisure,
  Backpacker,
  BusinessTraveler,
  ExecutiveBusiness,
  CoupleLeisure,
  FamilyLeisure,
  LuxuryLeisure,
  ConferenceDelegate,
  GroupTourTraveler,
  AirportTransitTraveler,
  WellnessTraveler,
  VipCelebrity,
  CriticReviewer
};
enum class GuestTrait : std::uint8_t {
  Patient,
  Impatient,
  Neat,
  Messy,
  LightSleeper,
  HeavySleeper,
  Foodie,
  Workaholic,
  Social,
  Private,
  Frugal,
  StatusConscious,
  FitnessFocused,
  EarlyRiser,
  NightOwl,
  ComplaintProne,
  Forgiving
};
constexpr std::uint32_t guestTraitFlag(GuestTrait trait) noexcept {
  return std::uint32_t{1} << static_cast<std::uint8_t>(trait);
}
enum class PersonState {
  OffDuty,
  Idle,
  Traveling,
  Working,
  Waiting,
  Sleeping,
  CheckedOut
};
enum class TaskKind { CheckIn, Turnover, Restock, Repair, CheckOut, Build };
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
struct GuestProfileView {
  GuestArchetype archetype{GuestArchetype::BudgetLeisure};
  std::int64_t budgetPerNightCents{16000};
  double priceSensitivity{0.5};
  double serviceSensitivity{0.5};
  double cleanlinessSensitivity{0.5};
  double noiseSensitivity{0.5};
  double privacySensitivity{0.5};
  double safetySensitivity{0.5};
  double comfortSensitivity{0.5};
  double foodSensitivity{0.5};
  double patience{0.5};
  std::uint32_t traitFlags{};
  bool operator==(const GuestProfileView &) const = default;
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
  int queueToleranceSeconds{};
  EntityId reservationId{};
  GuestProfileView profile;
  std::string goal;
  bool onShift{};
};
struct ReservationView {
  EntityId id{};
  std::string guestName;
  EntityId roomId{};
  int arrivalDay{};
  int departureDay{};
  std::int64_t nightlyRateCents{};
  double nightlyRate{};
  double satisfaction{70};
  std::int64_t checkInTravelSeconds{};
  int checkInWaitSeconds{};
  bool checkedIn{};
  bool checkoutStarted{};
  bool completed{};
  bool walkedRelocated{};
  GuestProfileView profile;
  std::string psychologyArchive;
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
};
struct ReviewView {
  EntityId reservationId{};
  int day{};
  double score{};
  std::string text;
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
  std::vector<RoomView> rooms;
  std::vector<PersonView> people;
  std::vector<ReservationView> reservations;
  std::vector<TaskView> tasks;
  std::vector<ReviewView> reviews;
  InventoryView inventory;
  EconomyView economy;
  std::vector<SupplyOrderView> supplyOrders;
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
  CommandResult buildFurnishedRoom(const RoomBlueprint &);
  CommandResult hireStaff(const StaffHire &);
  CommandResult fireStaff(EntityId employeeId);
  CommandResult setStaffShift(EntityId employeeId, int startHour, int endHour);
  CommandResult setRoomRate(EntityId roomId, double rate);
  CommandResult requestClean(EntityId roomId);
  CommandResult requestRepair(EntityId roomId);
  CommandResult closeRoom(EntityId roomId, bool closed);
  CommandResult removeRoom(EntityId roomId);
  CommandResult orderSupplies(const SupplyOrder &);
  CommandResult loadDefinitions(std::string_view jsonText);
  void setStaffOptimizerEnabled(bool enabled);

  [[nodiscard]] ConstructionPreview
  previewConstruction(const ConstructionCommand &) const;
  ConstructionResult executeConstruction(const ConstructionCommand &);
  BuildQueueResult queueBuild(const BuildPlan &);
  CommandResult cancelBuild(EntityId jobId);
  CommandResult addConstructionMaterials(const ConstructionMaterials &);
  [[nodiscard]] ConstructionSnapshot constructionSnapshot() const;

  CommandResult setRoomUtility(EntityId roomId, UtilityKind kind,
                               bool connected);
  CommandResult setRoomInfrastructure(EntityId roomId,
                                      InfrastructureKind kind, bool installed);
  [[nodiscard]] RoomSaleValidation validateRoomForSale(EntityId roomId) const;
  CommandResult installElevator(const ElevatorSpec &);
  CommandResult requestElevator(EntityId elevatorId, int pickupFloor,
                                int destinationFloor);
  [[nodiscard]] BuildingSystemsSnapshot buildingSystemsSnapshot() const;

  [[nodiscard]] GuestPsychologySnapshot guestPsychology(EntityId guestId) const;
  [[nodiscard]] GoalSelection
  chooseGuestGoal(EntityId guestId,
                  const GuestOpportunitySnapshot &opportunities) const;
  [[nodiscard]] SatisfactionBreakdown
  finalizeStaySatisfaction(EntityId guestId) const;
  CommandResult recordGuestExperience(EntityId guestId,
                                      const ExperienceEvent &event);

  void step(double seconds);

  [[nodiscard]] SimulationView view() const;
  [[nodiscard]] bool isReachable(Position from, Position to) const;
  [[nodiscard]] std::string save() const;
  static Simulation load(std::string_view data);

private:
  [[nodiscard]] std::string saveV11() const;
  static Simulation loadV11(std::string_view data);

  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace hh::game
