#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace hh::frontend {

using EntityId = std::uint64_t;

enum class SimulationSpeed : std::int64_t { Paused = 0, OneX = 1, TwoX = 2, FourX = 4, EightX = 8 };
enum class UiAction { NavigatePrevious, NavigateNext, Activate, Cancel, SpeedDown, SpeedUp, PauseToggle };
enum class InputModality { Mouse, Keyboard, Controller };
enum class HudFocus { Time, Speed, Alerts, Floor, Tool };
enum class InspectorKind { Room, Guest, Employee, Department, Task, Inventory, ServiceVenue, BuildJob, Elevator, BuildingSystem };
enum class ManagementPanelId : std::int64_t { None, Operations, Finance, Alerts, Objectives, Build, Rooms, Guests, Staff, Supplies };
enum class OverlayId : std::int64_t {
    None, Cleanliness, GuestSatisfaction, GuestTraffic, StaffTraffic, RoomQuality, RoomStatus,
    Noise, Temperature, ElectricalLoad, WaterDemand, MaintenanceCondition, OpenTaskDensity,
    StaffUtilization, QueueWait, ElevatorCongestion, FireSafety, SecurityCoverage, Revenue
};
enum class OperationArea { Staffing, Housekeeping, Laundry, Logistics, Engineering, FoodAndBeverage, Amenities, Events };
enum class AlertSeverity { Info, Warning, Critical };

enum class UiCommandType {
    None, SetSimulationSpeed, OpenInspector, OpenManagementPanel, SetOverlay, BuildConfirm,
    BuildRotate, BuildCancel, SetFutureRate, SetOverbookingPolicy, StartMarketingCampaign,
    AcceptContract, SaveGame, LoadGame, PauseGame, OpenSettings
};

struct UiCommand {
    UiCommandType type{UiCommandType::None};
    EntityId entityId{};
    std::int64_t integerValue{};
    std::int64_t secondaryIntegerValue{};
    std::string textValue;
    UiCommand() = default;
    UiCommand(UiCommandType commandType, EntityId entity = 0, std::int64_t integer = 0,
              std::int64_t secondaryInteger = 0, std::string text = {})
        : type(commandType), entityId(entity), integerValue(integer),
          secondaryIntegerValue(secondaryInteger), textValue(std::move(text)) {}
};

struct UiCommandResult { bool ok{}; std::string reasonCode; std::string message; };
struct HudSnapshot {
    std::int64_t cashCents{}; int day{}; int hour{}; int minute{};
    SimulationSpeed speed{SimulationSpeed::Paused}; int occupancyPermille{};
    int satisfactionPermille{}; int reputationPermille{}; int alertCount{};
    std::string currentTool{"Inspect"}; int activeFloor{}; bool cutaway{};
};
struct DiagnosticSnapshot { std::string code; std::string message; EntityId sourceEntityId{}; std::uint64_t causalParentId{}; };
struct FieldSnapshot { std::string label; std::string value; };
struct UiEntitySnapshot {
    EntityId id{}; InspectorKind kind{InspectorKind::Room}; std::string title;
    std::vector<FieldSnapshot> fields; std::vector<DiagnosticSnapshot> diagnostics;
};
struct BuildCatalogItem {
    std::string id; std::string name; std::string category; std::int64_t costCents{};
    std::vector<std::string> requiredMaterials; int laborMinutes{};
};
struct BuildPlacementPreview {
    std::uint64_t requestId{}; std::string itemId; bool valid{}; std::string reasonCode;
    std::string reasonText; std::int64_t costCents{}; std::vector<std::string> missingMaterials;
    int laborMinutes{}; int floor{}; int x{}; int y{}; int rotationQuarterTurns{};
};
struct OverlaySample { EntityId entityId{}; std::int64_t scaledValue{}; std::string exactText; std::string semanticText; };
struct OverlaySnapshot {
    OverlayId id{OverlayId::None}; std::string unit; std::int64_t minValue{}; std::int64_t maxValue{};
    std::vector<OverlaySample> samples;
};
struct OperationRow {
    EntityId id{}; OperationArea area{OperationArea::Staffing}; std::string name; std::string state;
    int priority{}; std::string reasonCode; std::int64_t ageSeconds{};
};
struct OperationsSnapshot {
    std::vector<OperationRow> rows; int scheduledStaff{}; int activeStaff{}; int fatiguedStaff{};
    int onBreakStaff{}; int housekeepingBacklog{}; int cleanLinenUnits{}; int laundryMachinesBusy{};
    int laundryMachinesTotal{}; int incomingOrders{}; int blockedInventoryMoves{};
    int engineeringOpenOrders{}; int roomServiceQueue{}; int foodTickets{}; int eventCount{};
    int amenityCapacityUsed{}; int amenityCapacityTotal{}; int serviceLevelPermille{};
};
struct EconomyKpiSnapshot {
    int todayOccupancyPermille{}; int sevenDayOccupancyPermille{}; int thirtyDayOccupancyPermille{};
    std::int64_t adrCents{}; std::int64_t revParCents{}; std::int64_t trevParCents{};
    std::int64_t gopCents{}; std::int64_t roomRevenueCents{}; std::int64_t totalRevenueCents{};
    std::int64_t laborCostCents{}; std::int64_t utilitiesCostCents{}; std::int64_t foodCostCents{};
    std::int64_t cashCents{}; std::int64_t cashRunwayDays{};
};
struct PricingRuleSnapshot {
    std::uint64_t ruleId{}; int startDay{}; int endDay{}; std::string roomCategory;
    std::int64_t rateCents{};
};
struct OverbookingPolicySnapshot {
    std::string roomCategory; int allowance{}; std::int64_t relocationCompensationCents{};
    int startDay{}; int endDay{};
};
struct EconomySnapshot {
    EconomyKpiSnapshot kpis; std::vector<FieldSnapshot> departmentContribution;
    std::vector<FieldSnapshot> bookingPace; std::vector<FieldSnapshot> cancellationAndNoShow;
    std::vector<FieldSnapshot> channelMix; std::vector<FieldSnapshot> competitors;
    std::vector<FieldSnapshot> demandBySegment; std::vector<FieldSnapshot> futureRateCalendar;
    std::vector<FieldSnapshot> campaigns; std::vector<FieldSnapshot> contracts;
    std::vector<FieldSnapshot> debtSchedule; std::vector<DiagnosticSnapshot> financingDiagnostics;
    std::vector<PricingRuleSnapshot> pricingRules;
    std::vector<OverbookingPolicySnapshot> overbookingPolicies;
};
struct AlertSnapshot {
    std::uint64_t id{}; AlertSeverity severity{AlertSeverity::Info}; EntityId sourceEntityId{};
    std::string reasonCode; std::string message; std::uint64_t causalParentId{}; bool resolved{};
};
struct ObjectiveSnapshot {
    std::uint64_t id{}; std::string title; std::int64_t current{}; std::int64_t target{}; bool complete{};
    std::string reasonCode; std::string reasonText; bool dismissible{};
};
struct ObjectivesSnapshot { std::vector<ObjectiveSnapshot> items; };
struct SimulationSnapshot {
    std::uint64_t revision{}; HudSnapshot hud; std::vector<UiEntitySnapshot> entities;
    std::vector<BuildCatalogItem> buildCatalog; BuildPlacementPreview buildPreview;
    std::vector<OverlaySnapshot> overlays; OperationsSnapshot operations; EconomySnapshot economy;
    std::vector<AlertSnapshot> alerts; ObjectivesSnapshot objectives;
};
struct GameUiSnapshot {
    std::uint64_t revision{}; HudSnapshot hud; std::vector<UiEntitySnapshot> entities;
    std::vector<BuildCatalogItem> buildCatalog; BuildPlacementPreview buildPreview;
    std::vector<OverlaySnapshot> overlays; OperationsSnapshot operations; EconomySnapshot economy;
    std::vector<AlertSnapshot> alerts; ObjectivesSnapshot objectives;
};
[[nodiscard]] GameUiSnapshot buildGameUiSnapshot(const SimulationSnapshot& snapshot);

} // namespace hh::frontend
