#include "GameUiBridge.h"

#include "hh/game/EconomyRuntime.h"
#include <algorithm>
#include <cstddef>
#include <cmath>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <utility>

namespace hh::client {
namespace {

using hh::frontend::AlertSeverity;
using hh::frontend::AlertSnapshot;
using hh::frontend::InspectorKind;
using hh::frontend::OperationArea;
using hh::frontend::OperationRow;
using hh::frontend::OverlayId;
using hh::frontend::OverlaySnapshot;
using hh::frontend::SimulationSpeed;
using hh::frontend::UiEntitySnapshot;

constexpr std::uint64_t FnvOffset = 1469598103934665603ULL;
constexpr std::uint64_t FnvPrime = 1099511628211ULL;

void hashValue(std::uint64_t& hash, std::uint64_t value) noexcept {
  for (int i = 0; i < 8; ++i) {
    hash ^= value & 0xffU;
    hash *= FnvPrime;
    value >>= 8U;
  }
}

void hashText(std::uint64_t& hash, const std::string& text) noexcept {
  for (const unsigned char c : text) {
    hash ^= c;
    hash *= FnvPrime;
  }
}

std::string cents(std::int64_t value) {
  const bool negative = value < 0;
  const std::uint64_t absolute =
      negative ? static_cast<std::uint64_t>(-(value + 1)) + 1U
               : static_cast<std::uint64_t>(value);
  std::string dollars = std::to_string(absolute / 100);
  for (std::ptrdiff_t i = static_cast<std::ptrdiff_t>(dollars.size()) - 3;
       i > 0; i -= 3) {
    dollars.insert(static_cast<std::size_t>(i), ",");
  }
  std::ostringstream out;
  if (negative)
    out << '-';
  out << '$' << dollars << '.' << std::setw(2) << std::setfill('0')
      << absolute % 100;
  return out.str();
}

std::string oneDecimal(double value) {
  std::ostringstream out;
  out << std::fixed << std::setprecision(1) << value;
  return out.str();
}

std::string percentage(double value) { return oneDecimal(value) + "%"; }

SimulationSpeed speedFromInt(int speed) noexcept {
  switch (speed) {
  case 0: return SimulationSpeed::Paused;
  case 1: return SimulationSpeed::OneX;
  case 2: return SimulationSpeed::TwoX;
  case 4: return SimulationSpeed::FourX;
  case 8: return SimulationSpeed::EightX;
  default: return SimulationSpeed::OneX;
  }
}

std::string roomStatusText(hh::game::RoomStatus status) {
  switch (status) {
  case hh::game::RoomStatus::Incomplete: return "Incomplete";
  case hh::game::RoomStatus::VacantReady: return "Ready to sell";
  case hh::game::RoomStatus::Reserved: return "Reserved";
  case hh::game::RoomStatus::Occupied: return "Occupied";
  case hh::game::RoomStatus::VacantDirty: return "Needs cleaning";
  case hh::game::RoomStatus::Cleaning: return "Cleaning";
  case hh::game::RoomStatus::OutOfOrder: return "Out of service";
  }
  return "Unknown";
}

std::string personStateText(hh::game::PersonState state) {
  switch (state) {
  case hh::game::PersonState::OffDuty: return "Off duty";
  case hh::game::PersonState::Idle: return "Idle";
  case hh::game::PersonState::Traveling: return "Traveling";
  case hh::game::PersonState::Working: return "Working";
  case hh::game::PersonState::Waiting: return "Waiting";
  case hh::game::PersonState::Sleeping: return "Sleeping";
  case hh::game::PersonState::CheckedOut: return "Checked out";
  }
  return "Unknown";
}

std::string personKindText(hh::game::PersonKind kind) {
  switch (kind) {
  case hh::game::PersonKind::Guest: return "Guest";
  case hh::game::PersonKind::Receptionist: return "Reception";
  case hh::game::PersonKind::Housekeeper: return "Housekeeping";
  case hh::game::PersonKind::Maintenance: return "Engineering";
  }
  return "Person";
}

InspectorKind inspectorKind(hh::game::PersonKind kind) noexcept {
  return kind == hh::game::PersonKind::Guest ? InspectorKind::Guest
                                             : InspectorKind::Employee;
}

std::string taskKindText(hh::game::TaskKind kind) {
  switch (kind) {
  case hh::game::TaskKind::CheckIn: return "Check-in";
  case hh::game::TaskKind::Turnover: return "Room turnover";
  case hh::game::TaskKind::Restock: return "Restock";
  case hh::game::TaskKind::Repair: return "Repair";
  case hh::game::TaskKind::CheckOut: return "Check-out";
  }
  return "Task";
}

std::string taskStatusText(hh::game::TaskStatus status) {
  switch (status) {
  case hh::game::TaskStatus::Ready: return "Ready";
  case hh::game::TaskStatus::Traveling: return "Traveling";
  case hh::game::TaskStatus::Working: return "Working";
  case hh::game::TaskStatus::Blocked: return "Blocked";
  case hh::game::TaskStatus::Completed: return "Completed";
  }
  return "Unknown";
}

OperationArea taskArea(hh::game::TaskKind kind) noexcept {
  switch (kind) {
  case hh::game::TaskKind::Turnover: return OperationArea::Housekeeping;
  case hh::game::TaskKind::Restock: return OperationArea::Logistics;
  case hh::game::TaskKind::Repair: return OperationArea::Engineering;
  case hh::game::TaskKind::CheckIn:
  case hh::game::TaskKind::CheckOut: return OperationArea::Staffing;
  }
  return OperationArea::Staffing;
}

std::string blockReasonCode(hh::game::BlockReason reason) {
  switch (reason) {
  case hh::game::BlockReason::None: return {};
  case hh::game::BlockReason::MissingCleanLinen: return "MISSING_CLEAN_LINEN";
  case hh::game::BlockReason::MissingTowels: return "MISSING_TOWELS";
  case hh::game::BlockReason::MissingAmenities: return "MISSING_AMENITIES";
  case hh::game::BlockReason::MissingChemicals: return "MISSING_CHEMICALS";
  case hh::game::BlockReason::MissingWasher: return "MISSING_WASHER";
  case hh::game::BlockReason::MissingDryer: return "MISSING_DRYER";
  case hh::game::BlockReason::MissingFoldingStation: return "MISSING_FOLDING_STATION";
  case hh::game::BlockReason::MissingCleanStorage: return "MISSING_CLEAN_STORAGE";
  case hh::game::BlockReason::MissingDirtyStorage: return "MISSING_DIRTY_STORAGE";
  case hh::game::BlockReason::InsufficientStorageCapacity: return "INSUFFICIENT_STORAGE_CAPACITY";
  case hh::game::BlockReason::MissingReceiving: return "MISSING_RECEIVING";
  case hh::game::BlockReason::MissingRoute: return "MISSING_ROUTE";
  case hh::game::BlockReason::MissingWasteCapacity: return "MISSING_WASTE_CAPACITY";
  case hh::game::BlockReason::AwaitingPart: return "AWAITING_PART";
  case hh::game::BlockReason::AwaitingProduction: return "AWAITING_PRODUCTION";
  }
  return "UNKNOWN_BLOCK_REASON";
}

std::string foodBlockCode(hh::game::FoodBlockReason reason) {
  switch (reason) {
  case hh::game::FoodBlockReason::None: return {};
  case hh::game::FoodBlockReason::MissingRecipe: return "FNB_MISSING_RECIPE";
  case hh::game::FoodBlockReason::InvalidOrder: return "FNB_INVALID_ORDER";
  case hh::game::FoodBlockReason::MissingIngredient: return "FNB_MISSING_INGREDIENT";
  case hh::game::FoodBlockReason::MissingStation: return "FNB_MISSING_STATION";
  case hh::game::FoodBlockReason::StationCapacity: return "FNB_STATION_CAPACITY";
  case hh::game::FoodBlockReason::NoStaffCapacity: return "FNB_NO_STAFF_CAPACITY";
  case hh::game::FoodBlockReason::OutsideServiceWindow: return "FNB_OUTSIDE_SERVICE_WINDOW";
  case hh::game::FoodBlockReason::VenueCapacity: return "FNB_VENUE_CAPACITY";
  case hh::game::FoodBlockReason::MissingVenue: return "FNB_MISSING_VENUE";
  }
  return "FNB_UNKNOWN_BLOCK";
}

std::string foodStageText(hh::game::FoodStage stage) {
  switch (stage) {
  case hh::game::FoodStage::Queued: return "Queued";
  case hh::game::FoodStage::Blocked: return "Blocked";
  case hh::game::FoodStage::AwaitingSeat: return "Awaiting seat";
  case hh::game::FoodStage::Seated: return "Seated";
  case hh::game::FoodStage::Ordered: return "Ordered";
  case hh::game::FoodStage::Prep: return "Prep";
  case hh::game::FoodStage::Cook: return "Cook";
  case hh::game::FoodStage::Plate: return "Plate";
  case hh::game::FoodStage::Ready: return "Ready";
  case hh::game::FoodStage::Served: return "Served";
  case hh::game::FoodStage::Paid: return "Paid";
  case hh::game::FoodStage::Completed: return "Completed";
  case hh::game::FoodStage::Cancelled: return "Cancelled";
  }
  return "Unknown";
}

bool terminalFoodStage(hh::game::FoodStage stage) noexcept {
  return stage == hh::game::FoodStage::Completed ||
         stage == hh::game::FoodStage::Cancelled;
}

std::string distressText(hh::game::DistressStage stage) {
  switch (stage) {
  case hh::game::DistressStage::Healthy: return "Healthy";
  case hh::game::DistressStage::Tight: return "Tight";
  case hh::game::DistressStage::Critical: return "Critical";
  case hh::game::DistressStage::Insolvent: return "Insolvent";
  case hh::game::DistressStage::Default: return "Default";
  case hh::game::DistressStage::Receivership: return "Receivership";
  }
  return "Unknown";
}

std::string marketPositionText(hh::game::PricePosition position) {
  switch (position) {
  case hh::game::PricePosition::Unknown: return "Unknown";
  case hh::game::PricePosition::BelowMarket: return "Below market";
  case hh::game::PricePosition::AtMarket: return "At market";
  case hh::game::PricePosition::AboveMarket: return "Above market";
  }
  return "Unknown";
}

std::uint64_t alertId(std::uint64_t prefix, std::uint64_t entity) noexcept {
  return prefix ^ entity;
}

OverlaySnapshot makeOverlay(OverlayId id, std::string unit,
                            std::int64_t minValue, std::int64_t maxValue) {
  OverlaySnapshot overlay;
  overlay.id = id;
  overlay.unit = std::move(unit);
  overlay.minValue = minValue;
  overlay.maxValue = maxValue;
  return overlay;
}

void mapEconomy(const hh::game::SimulationView& view,
                const hh::game::EconomyRuntime* runtime,
                hh::frontend::EconomySnapshot& output) {
  output.kpis.todayOccupancyPermille =
      static_cast<int>(std::lround(view.economy.occupancy * 1000.0));
  output.kpis.cashCents = view.economy.cashCents;
  output.kpis.roomRevenueCents = view.economy.revenueCents;
  output.kpis.totalRevenueCents = view.economy.revenueCents;
  output.kpis.laborCostCents = view.economy.payrollCents;
  output.kpis.utilitiesCostCents = view.economy.utilityCostCents;

  if (runtime == nullptr)
    return;

  const auto financial = runtime->financialSnapshot();
  const auto revenue = runtime->revenueManagementSnapshot();
  const auto market = runtime->marketSnapshot();
  const auto commercial = runtime->commercialSnapshot();

  output.kpis.cashCents = financial.economics.cashCents;
  output.kpis.totalRevenueCents = financial.economics.totalRevenueCents;
  output.kpis.roomRevenueCents = financial.economics.roomRevenueCents;
  output.kpis.gopCents = financial.economics.gopCents;
  output.kpis.adrCents = financial.economics.adrCents;
  output.kpis.revParCents = financial.economics.revParCents;
  output.kpis.trevParCents = financial.economics.tRevParCents;
  output.kpis.todayOccupancyPermille =
      static_cast<int>(std::lround(financial.economics.occupancy * 1000.0));

  output.debtSchedule.push_back(
      {"Outstanding principal", cents(financial.financing.outstandingPrincipalCents)});
  output.debtSchedule.push_back(
      {"Next debt service", cents(financial.financing.nextDebtServiceCents)});
  output.debtSchedule.push_back(
      {"Next payment day", std::to_string(financial.financing.nextPaymentDay)});
  output.debtSchedule.push_back(
      {"Distress state", distressText(financial.financing.distressStage)});
  if (financial.financing.covenantBreach) {
    output.financingDiagnostics.push_back(
        {"FIN_COVENANT_BREACH", "Loan covenant breached", 0, 0});
  }
  if (financial.financing.missedObligationCents > 0) {
    output.financingDiagnostics.push_back(
        {"FIN_MISSED_OBLIGATION",
         "Missed obligation " + cents(financial.financing.missedObligationCents),
         0, 0});
  }

  for (const auto& competitor : market.competitors) {
    output.competitors.push_back(
        {competitor.name, cents(competitor.nightlyRateCents)});
  }
  output.bookingPace.push_back(
      {"Generated requests", std::to_string(market.generatedRequests)});
  output.bookingPace.push_back(
      {"Player wins", std::to_string(market.playerWins)});
  output.bookingPace.push_back(
      {"Competitor wins", std::to_string(market.competitorWins)});

  for (const auto& [category, rate] : revenue.effectiveRateCents) {
    const auto position = revenue.pricePositions.find(category);
    const std::string suffix =
        position == revenue.pricePositions.end()
            ? std::string{}
            : " · " + marketPositionText(position->second);
    output.futureRateCalendar.push_back({category, cents(rate) + suffix});
  }

  for (const auto& campaign : commercial.campaigns) {
    output.campaigns.push_back(
        {"Campaign " + std::to_string(campaign.id),
         "Day " + std::to_string(campaign.startDay) + "–" +
             std::to_string(campaign.endDay) + " · " + cents(campaign.costCents)});
  }
  for (const auto& accepted : commercial.contracts) {
    output.contracts.push_back(
        {"Contract " + std::to_string(accepted.contract.id),
         cents(accepted.contract.negotiatedRateCents) +
             (accepted.acceptedRisk ? " · risk accepted" : "")});
  }
}

} // namespace

hh::frontend::SimulationSnapshot
makeGameUiSnapshotSource(const hh::game::Simulation& simulation,
                         const GameUiBridgeContext& context) {
  const auto view = simulation.view();
  const auto logistics = simulation.logisticsSnapshot();
  const auto food = simulation.foodServiceSnapshot();
  const auto events = simulation.eventsSnapshot();
  const auto amenities = simulation.amenitiesSnapshot();

  hh::frontend::SimulationSnapshot out;
  std::uint64_t revision = FnvOffset;

  out.hud.cashCents = view.economy.cashCents;
  out.hud.day = view.day;
  out.hud.hour = view.hour;
  out.hud.minute = static_cast<int>((view.elapsedSeconds / 60) % 60);
  out.hud.speed = speedFromInt(context.simulationSpeed);
  out.hud.occupancyPermille =
      static_cast<int>(std::lround(view.economy.occupancy * 1000.0));
  out.hud.reputationPermille =
      static_cast<int>(std::lround(view.economy.reputation * 10.0));
  out.hud.currentTool = context.currentTool;
  out.hud.activeFloor = context.activeFloor;
  out.hud.cutaway = context.cutaway;
  out.buildPreview = context.buildPreview;

  auto cleanliness = makeOverlay(OverlayId::Cleanliness, "%", 0, 1000);
  auto satisfaction = makeOverlay(OverlayId::GuestSatisfaction, "%", 0, 1000);
  auto roomQuality = makeOverlay(OverlayId::RoomQuality, "score", 0, 1000);
  auto roomStatus = makeOverlay(OverlayId::RoomStatus, "status", 0, 7);
  auto maintenance = makeOverlay(OverlayId::MaintenanceCondition, "%", 0, 1000);
  auto openTasks = makeOverlay(OverlayId::OpenTaskDensity, "tasks", 0, 1000);
  auto staffUtilization = makeOverlay(OverlayId::StaffUtilization, "%", 0, 1000);
  auto queueWait = makeOverlay(OverlayId::QueueWait, "seconds", 0, 86400);

  double guestSatisfactionTotal = 0.0;
  int guestSatisfactionCount = 0;

  for (const auto& room : view.rooms) {
    UiEntitySnapshot entity;
    entity.id = room.id;
    entity.kind = InspectorKind::Room;
    entity.title = room.name;
    entity.fields = {
        {"Status", roomStatusText(room.status)},
        {"Floor", std::to_string(room.floor)},
        {"Cleanliness", percentage(room.cleanliness)},
        {"Condition", percentage(room.condition)},
        {"Nightly rate", cents(room.nightlyRateCents)},
        {"Reachable", room.reachable ? "Yes" : "No"},
        {"Closed", room.closed ? "Yes" : "No"}};

    if (!room.reachable) {
      entity.diagnostics.push_back(
          {"ROOM_UNREACHABLE", "No route to the guest entrance", room.id, 0});
      out.alerts.push_back(
          {alertId(0x1100000000000000ULL, room.id), AlertSeverity::Critical,
           room.id, "ROOM_UNREACHABLE", "Room has no guest-access route", 0,
           false});
    }
    if (room.status == hh::game::RoomStatus::OutOfOrder) {
      entity.diagnostics.push_back(
          {"ROOM_OUT_OF_ORDER", "Room is out of service", room.id, 0});
      out.alerts.push_back(
          {alertId(0x1200000000000000ULL, room.id), AlertSeverity::Warning,
           room.id, "ROOM_OUT_OF_ORDER", "Room is out of service", 0, false});
    }
    if (room.closed) {
      entity.diagnostics.push_back(
          {"ROOM_CLOSED", "Room is closed by player policy", room.id, 0});
    }
    out.entities.push_back(std::move(entity));

    const auto clean =
        static_cast<std::int64_t>(std::lround(room.cleanliness * 10.0));
    cleanliness.samples.push_back(
        {room.id, clean, percentage(room.cleanliness),
         room.cleanliness < 70.0 ? "Needs cleaning" : "Clean"});
    const auto condition =
        static_cast<std::int64_t>(std::lround(room.condition * 10.0));
    roomQuality.samples.push_back(
        {room.id, condition, percentage(room.condition),
         room.condition < 60.0 ? "Low condition" : "Good condition"});
    maintenance.samples.push_back(
        {room.id, condition, percentage(room.condition),
         room.condition < 60.0 ? "Maintenance attention" : "Serviceable"});
    roomStatus.samples.push_back(
        {room.id, static_cast<std::int64_t>(room.status), roomStatusText(room.status),
         roomStatusText(room.status)});

    hashValue(revision, room.id);
    hashValue(revision, static_cast<std::uint64_t>(room.status));
    hashValue(revision, static_cast<std::uint64_t>(room.nightlyRateCents));
    hashValue(revision, static_cast<std::uint64_t>(std::lround(room.cleanliness * 100.0)));
    hashValue(revision, static_cast<std::uint64_t>(std::lround(room.condition * 100.0)));
    hashValue(revision, room.reachable ? 1U : 0U);
    hashValue(revision, room.closed ? 1U : 0U);
  }

  for (const auto& person : view.people) {
    UiEntitySnapshot entity;
    entity.id = person.id;
    entity.kind = inspectorKind(person.kind);
    entity.title = person.name;
    entity.fields = {
        {"Role", personKindText(person.kind)},
        {"State", personStateText(person.state)},
        {"Goal", person.goal},
        {"Floor", std::to_string(person.position.floor)},
        {"Fatigue", percentage(person.fatigue)},
        {"Satisfaction", percentage(person.satisfaction)},
        {"Queue wait", std::to_string(person.queueWaitSeconds) + " s"},
        {"Travel", std::to_string(person.travelSeconds) + " s"}};
    if (person.kind != hh::game::PersonKind::Guest) {
      entity.fields.push_back(
          {"Shift", std::to_string(person.shiftStartHour) + ":00–" +
                        std::to_string(person.shiftEndHour) + ":00"});
      entity.fields.push_back({"On shift", person.onShift ? "Yes" : "No"});
      entity.fields.push_back({"Hourly wage", cents(person.hourlyWageCents)});
    }
    out.entities.push_back(std::move(entity));

    if (person.kind == hh::game::PersonKind::Guest) {
      guestSatisfactionTotal += person.satisfaction;
      ++guestSatisfactionCount;
      satisfaction.samples.push_back(
          {person.id,
           static_cast<std::int64_t>(std::lround(person.satisfaction * 10.0)),
           percentage(person.satisfaction),
           person.satisfaction < 60.0 ? "Dissatisfied" : "Satisfied"});
    } else {
      const bool busy = person.state == hh::game::PersonState::Working ||
                        person.state == hh::game::PersonState::Traveling;
      staffUtilization.samples.push_back(
          {person.id, busy ? 1000 : 0, busy ? "Active" : "Not active",
           personStateText(person.state)});
      if (person.onShift)
        ++out.operations.activeStaff;
      ++out.operations.scheduledStaff;
      if (person.fatigue >= 75.0)
        ++out.operations.fatiguedStaff;
    }

    if (person.queueWaitSeconds > 0) {
      queueWait.samples.push_back(
          {person.id, person.queueWaitSeconds,
           std::to_string(person.queueWaitSeconds) + " s",
           person.queueWaitSeconds >= 300 ? "Long wait" : "Waiting"});
    }

    hashValue(revision, person.id);
    hashValue(revision, static_cast<std::uint64_t>(person.state));
    hashValue(revision, static_cast<std::uint64_t>(std::lround(person.satisfaction * 100.0)));
    hashValue(revision, static_cast<std::uint64_t>(person.queueWaitSeconds));
  }

  if (guestSatisfactionCount > 0) {
    out.hud.satisfactionPermille = static_cast<int>(std::lround(
        guestSatisfactionTotal * 10.0 / static_cast<double>(guestSatisfactionCount)));
  }

  std::map<hh::frontend::EntityId, int> taskCountByTarget;
  for (const auto& task : view.tasks) {
    UiEntitySnapshot entity;
    entity.id = task.id;
    entity.kind = InspectorKind::Task;
    entity.title = taskKindText(task.kind) + " #" + std::to_string(task.id);
    entity.fields = {
        {"Status", taskStatusText(task.status)},
        {"Target", std::to_string(task.targetId)},
        {"Employee", std::to_string(task.employeeId)},
        {"Remaining", oneDecimal(task.workRemainingSeconds) + " s"}};
    if (task.status == hh::game::TaskStatus::Blocked) {
      const auto code =
          task.blockedReason.empty() ? std::string{"TASK_BLOCKED"}
                                     : task.blockedReason;
      const auto message =
          task.blockedReason.empty() ? std::string{"Task is blocked"}
                                     : task.blockedReason;
      entity.diagnostics.push_back({code, message, task.targetId, 0});
      out.alerts.push_back(
          {alertId(0x2100000000000000ULL, task.id), AlertSeverity::Warning,
           task.targetId != 0 ? task.targetId : task.id, code, message, 0, false});
    }
    out.entities.push_back(std::move(entity));

    out.operations.rows.push_back(
        {task.id, taskArea(task.kind), taskKindText(task.kind),
         taskStatusText(task.status),
         task.status == hh::game::TaskStatus::Blocked ? 3 : 1,
         task.blockedReason, 0});
    if (task.kind == hh::game::TaskKind::Turnover &&
        task.status != hh::game::TaskStatus::Completed) {
      ++out.operations.housekeepingBacklog;
    }
    if (task.kind == hh::game::TaskKind::Repair &&
        task.status != hh::game::TaskStatus::Completed) {
      ++out.operations.engineeringOpenOrders;
    }
    if (task.status != hh::game::TaskStatus::Completed)
      ++taskCountByTarget[task.targetId];

    hashValue(revision, task.id);
    hashValue(revision, static_cast<std::uint64_t>(task.status));
    hashText(revision, task.blockedReason);
  }

  for (const auto& [target, count] : taskCountByTarget) {
    openTasks.samples.push_back(
        {target, count, std::to_string(count) + " open",
         count > 2 ? "High task density" : "Open tasks"});
  }

  UiEntitySnapshot inventoryEntity;
  inventoryEntity.id = 0xF000000000000001ULL;
  inventoryEntity.kind = InspectorKind::Inventory;
  inventoryEntity.title = "Property inventory";
  inventoryEntity.fields = {
      {"Linen", std::to_string(view.inventory.linen)},
      {"Towels", std::to_string(view.inventory.towels)},
      {"Amenities", std::to_string(view.inventory.amenities)},
      {"Chemicals", std::to_string(view.inventory.chemicals)},
      {"Parts", std::to_string(view.inventory.parts)}};
  out.entities.push_back(std::move(inventoryEntity));

  for (const auto& order : view.supplyOrders) {
    if (!order.delivered)
      ++out.operations.incomingOrders;
  }

  for (const auto& stack : logistics.inventory) {
    if (stack.item == "clean_linen")
      out.operations.cleanLinenUnits += stack.quantity - stack.reservedQuantity;
  }
  for (const auto& order : logistics.purchaseOrders) {
    if (order.state != hh::game::PurchaseOrderState::Completed &&
        order.state != hh::game::PurchaseOrderState::Cancelled) {
      ++out.operations.incomingOrders;
      const auto code = blockReasonCode(order.blockedReason);
      out.operations.rows.push_back(
          {order.id, OperationArea::Logistics,
           "Purchase order · " + order.item,
           code.empty() ? "In progress" : "Blocked",
           code.empty() ? 1 : 3, code, 0});
      if (!code.empty())
        ++out.operations.blockedInventoryMoves;
    }
  }
  for (const auto& move : logistics.stockMoves) {
    if (move.completed)
      continue;
    const auto code = blockReasonCode(move.blockedReason);
    out.operations.rows.push_back(
        {move.id, OperationArea::Logistics, "Inventory move · " + move.item,
         code.empty() ? "In progress" : "Blocked", code.empty() ? 1 : 3, code,
         0});
    if (!code.empty())
      ++out.operations.blockedInventoryMoves;
  }

  for (const auto& order : food.orders) {
    if (terminalFoodStage(order.stage))
      continue;
    ++out.operations.foodTickets;
    const auto code = foodBlockCode(order.blockReason);
    out.operations.rows.push_back(
        {order.id, OperationArea::FoodAndBeverage,
         "F&B ticket #" + std::to_string(order.id), foodStageText(order.stage),
         code.empty() ? 1 : 3, code, order.ageSeconds});
    if (!code.empty()) {
      out.alerts.push_back(
          {alertId(0x3100000000000000ULL, order.id), AlertSeverity::Warning,
           order.id, code, "Food-service order blocked", 0, false});
    }
  }

  for (const auto& booking : events.bookings) {
    if (booking.phase != hh::game::EventPhase::Completed &&
        booking.phase != hh::game::EventPhase::Cancelled) {
      ++out.operations.eventCount;
      out.operations.rows.push_back(
          {booking.id, OperationArea::Events,
           "Event #" + std::to_string(booking.id), "Active", 1, "", 0});
    }
  }

  for (const auto& amenity : amenities.amenities) {
    out.operations.amenityCapacityUsed += amenity.active;
    out.operations.amenityCapacityTotal += amenity.capacity;
    UiEntitySnapshot entity;
    entity.id = amenity.id;
    entity.kind = InspectorKind::ServiceVenue;
    entity.title = "Amenity #" + std::to_string(amenity.id);
    entity.fields = {
        {"Open", amenity.open ? "Yes" : "No"},
        {"Capacity", std::to_string(amenity.active) + "/" +
                         std::to_string(amenity.capacity)},
        {"Cleanliness",
         percentage(static_cast<double>(amenity.cleanliness) / 100.0)},
        {"Condition", percentage(static_cast<double>(amenity.condition) / 100.0)},
        {"Price", cents(amenity.priceCents)}};
    if (amenity.cleaningRequired)
      entity.diagnostics.push_back(
          {"AMENITY_CLEANING_REQUIRED", "Cleaning required", amenity.id, 0});
    if (amenity.maintenanceRequired)
      entity.diagnostics.push_back(
          {"AMENITY_MAINTENANCE_REQUIRED", "Maintenance required", amenity.id, 0});
    out.entities.push_back(std::move(entity));
  }

  const auto activeRows = static_cast<int>(out.operations.rows.size());
  const auto blockedRows = static_cast<int>(std::count_if(
      out.operations.rows.begin(), out.operations.rows.end(),
      [](const OperationRow& row) { return !row.reasonCode.empty(); }));
  out.operations.serviceLevelPermille =
      activeRows == 0 ? 1000
                      : std::max(0, 1000 - (blockedRows * 1000 / activeRows));

  mapEconomy(view, context.economy, out.economy);

  out.hud.alertCount = static_cast<int>(out.alerts.size());
  out.overlays.push_back(std::move(cleanliness));
  if (!satisfaction.samples.empty())
    out.overlays.push_back(std::move(satisfaction));
  out.overlays.push_back(std::move(roomQuality));
  out.overlays.push_back(std::move(roomStatus));
  out.overlays.push_back(std::move(maintenance));
  if (!openTasks.samples.empty())
    out.overlays.push_back(std::move(openTasks));
  if (!staffUtilization.samples.empty())
    out.overlays.push_back(std::move(staffUtilization));
  if (!queueWait.samples.empty())
    out.overlays.push_back(std::move(queueWait));

  hashValue(revision, static_cast<std::uint64_t>(view.elapsedSeconds));
  hashValue(revision, static_cast<std::uint64_t>(view.economy.cashCents));
  hashValue(revision, static_cast<std::uint64_t>(out.alerts.size()));
  hashValue(revision, static_cast<std::uint64_t>(context.simulationSpeed));
  hashValue(revision, static_cast<std::uint64_t>(context.activeFloor));
  hashText(revision, context.currentTool);
  hashValue(revision, context.buildPreview.requestId);
  hashValue(revision, context.buildPreview.valid ? 1U : 0U);
  hashText(revision, context.buildPreview.reasonCode);
  out.revision = revision;
  return out;
}

} // namespace hh::client
