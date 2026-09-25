#include "hh/game/Simulation.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace hh::game;

static void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

static const RoomView &room(const SimulationView &view, EntityId id) {
  const auto it = std::find_if(view.rooms.begin(), view.rooms.end(),
                               [id](const RoomView &candidate) {
                                 return candidate.id == id;
                               });
  if (it == view.rooms.end())
    throw std::runtime_error("room not found in simulation view");
  return *it;
}

static ServiceLogisticsRuntime serviceRuntime(const Simulation &simulation) {
  const auto state = simulation.save();
  const auto headerStart = state.find("FINAL04 ");
  if (headerStart == std::string::npos)
    throw std::runtime_error("FINAL-04 section missing from simulation save");
  const auto headerEnd = state.find('\n', headerStart);
  if (headerEnd == std::string::npos)
    throw std::runtime_error("FINAL-04 section header is truncated");
  std::istringstream header(state.substr(headerStart, headerEnd - headerStart));
  std::string tag;
  std::size_t bytes{};
  header >> tag >> bytes;
  if (!header || tag != "FINAL04")
    throw std::runtime_error("FINAL-04 section header could not be parsed");
  const auto payloadStart = headerEnd + 1;
  if (bytes > state.size() - payloadStart)
    throw std::runtime_error("FINAL-04 section payload is truncated");
  return ServiceLogisticsRuntime::load(state.substr(payloadStart, bytes));
}

static const WorkOrderView &workOrder(const EngineeringSnapshot &snapshot,
                                      WorkOrderId id) {
  const auto it = std::find_if(snapshot.workOrders.begin(),
                               snapshot.workOrders.end(),
                               [id](const WorkOrderView &candidate) {
                                 return candidate.id == id;
                               });
  if (it == snapshot.workOrders.end())
    throw std::runtime_error("work order missing from engineering snapshot");
  return *it;
}

static const EngineeringAssetView &
engineeringAsset(const EngineeringSnapshot &snapshot, AssetId id) {
  const auto it = std::find_if(snapshot.assets.begin(), snapshot.assets.end(),
                               [id](const EngineeringAssetView &candidate) {
                                 return candidate.id == id;
                               });
  if (it == snapshot.assets.end())
    throw std::runtime_error("asset missing from engineering snapshot");
  return *it;
}

static int itemTotal(const LogisticsSnapshot &snapshot,
                     const char *item) {
  int total = 0;
  for (const auto &stack : snapshot.inventory)
    if (stack.item == item)
      total += stack.quantity;
  return total;
}

static int itemAt(const LogisticsSnapshot &snapshot, StorageKind kind,
                  const char *item) {
  int total = 0;
  for (const auto &node : snapshot.storage) {
    if (node.kind != kind)
      continue;
    for (const auto &stack : snapshot.inventory)
      if (stack.storage == node.id && stack.item == item)
        total += stack.quantity;
  }
  return total;
}

static std::string transformFinal04(
    const std::string &simulationState,
    const std::function<std::string(const std::string &)> &transform) {
  const auto headerStart = simulationState.find("FINAL04 ");
  if (headerStart == std::string::npos)
    throw std::runtime_error("FINAL-04 section missing from simulation save");
  const auto headerEnd = simulationState.find('\n', headerStart);
  if (headerEnd == std::string::npos)
    throw std::runtime_error("FINAL-04 section header is truncated");

  std::istringstream header(
      simulationState.substr(headerStart, headerEnd - headerStart));
  std::string tag;
  std::size_t bytes{};
  header >> tag >> bytes;
  if (!header || tag != "FINAL04")
    throw std::runtime_error("FINAL-04 section header could not be parsed");

  const auto stateStart = headerEnd + 1;
  if (bytes > simulationState.size() - stateStart)
    throw std::runtime_error("FINAL-04 section payload is truncated");
  const auto stateEnd = stateStart + bytes;
  if (stateEnd >= simulationState.size() || simulationState[stateEnd] != '\n')
    throw std::runtime_error("FINAL-04 section terminator is missing");

  const auto replacement =
      transform(simulationState.substr(stateStart, bytes));
  std::ostringstream rebuilt;
  rebuilt << simulationState.substr(0, headerStart) << "FINAL04 "
          << replacement.size() << '\n' << replacement << '\n'
          << simulationState.substr(stateEnd + 1);
  return rebuilt.str();
}

static void requireSimulationLoadRejected(const std::string &encoded,
                                          const char *message) {
  bool rejected = false;
  try {
    (void)Simulation::load(encoded);
  } catch (const std::invalid_argument &) {
    rejected = true;
  }
  require(rejected, message);
}

static std::string replaceLegacyInventoryLine(const std::string &encoded,
                                              const std::string &replacement) {
  std::size_t lineStart = 0;
  for (int line = 0; line < 3; ++line) {
    const auto end = encoded.find('\n', lineStart);
    if (end == std::string::npos)
      throw std::runtime_error("simulation save header is truncated");
    lineStart = end + 1;
  }
  const auto lineEnd = encoded.find('\n', lineStart);
  if (lineEnd == std::string::npos)
    throw std::runtime_error("simulation inventory line is truncated");
  return encoded.substr(0, lineStart) + replacement +
         encoded.substr(lineEnd);
}

static void stable_commands_and_save_boundary() {
  auto sim = Simulation::tutorial(321);
  const auto view = sim.view();
  require(!view.rooms.empty(), "tutorial requires a serviceable room");
  require(sim.logisticsSnapshot().elapsedSeconds == view.elapsedSeconds,
          "tutorial FINAL-04 clock did not start on the simulation clock");
  const auto room = view.rooms.front().id;

  const auto turn = sim.requestRoomTurn(room);
  require(turn != 0, "Simulation did not expose room-turn command");

  const auto work = sim.createWorkOrder(room, WorkOrderType::Preventive);
  require(work != 0, "Simulation did not expose engineering command");

  RoomServiceOrder order;
  const auto roomService = sim.placeRoomServiceOrder(7001, order);
  require(roomService != 0, "Simulation did not expose room-service command");

  const auto before = sim.logisticsSnapshot().elapsedSeconds;
  sim.step(10.0);
  const auto after = sim.logisticsSnapshot().elapsedSeconds;
  require(after - before == 10,
          "service/logistics clock is not driven by Simulation");

  const auto encoded = sim.save();
  auto restored = Simulation::load(encoded);
  require(restored.logisticsSnapshot().elapsedSeconds == after,
          "Simulation save/load dropped FINAL-04 state");

  auto mirroredClean = Simulation::tutorial(326);
  const auto mirroredRoom = mirroredClean.view().rooms.front().id;
  const auto housekeepingBefore =
      mirroredClean.housekeepingSnapshot().jobs.size();
  require(mirroredClean.requestClean(mirroredRoom).ok,
          "physical clean request was rejected");
  const auto mirroredHousekeeping = mirroredClean.housekeepingSnapshot();
  require(mirroredHousekeeping.jobs.size() == housekeepingBefore + 1,
          "physical clean request did not create a FINAL-04 room turn");
  require(mirroredHousekeeping.jobs.back().roomId == mirroredRoom &&
              mirroredHousekeeping.jobs.back().stage !=
                  HousekeepingStage::Completed,
          "mirrored FINAL-04 room turn targeted the wrong room");
  const auto mirroredCleanViewAfterFirst = mirroredClean.view();
  const auto physicalTurnsAfterFirst = std::count_if(
      mirroredCleanViewAfterFirst.tasks.begin(),
      mirroredCleanViewAfterFirst.tasks.end(),
      [mirroredRoom](const auto &task) {
        return task.targetId == mirroredRoom &&
               task.kind == TaskKind::Turnover &&
               task.status != TaskStatus::Completed;
      });
  require(mirroredClean.requestClean(mirroredRoom).ok,
          "duplicate physical clean request unexpectedly failed");
  require(mirroredClean.housekeepingSnapshot().jobs.size() ==
              housekeepingBefore + 1,
          "duplicate physical clean request created a second FINAL-04 job");
  const auto mirroredCleanViewAfterSecond = mirroredClean.view();
  const auto physicalTurnsAfterSecond = std::count_if(
      mirroredCleanViewAfterSecond.tasks.begin(),
      mirroredCleanViewAfterSecond.tasks.end(),
      [mirroredRoom](const auto &task) {
        return task.targetId == mirroredRoom &&
               task.kind == TaskKind::Turnover &&
               task.status != TaskStatus::Completed;
      });
  require(physicalTurnsAfterFirst == 1 && physicalTurnsAfterSecond == 1,
          "duplicate physical clean request created duplicate worker tasks");

  auto preclaimPersistence = Simulation::tutorial(334);
  const auto preclaimRoom = preclaimPersistence.view().rooms.front().id;
  const auto preclaimInventoryBefore = preclaimPersistence.view().inventory;
  require(preclaimPersistence.requestClean(preclaimRoom).ok,
          "preclaim persistence clean request was rejected");
  preclaimPersistence.step(20);
  const auto preclaimInventoryAfter = preclaimPersistence.view().inventory;
  require(preclaimInventoryAfter.linen == preclaimInventoryBefore.linen - 1 &&
              preclaimInventoryAfter.towels ==
                  preclaimInventoryBefore.towels - 2 &&
              preclaimInventoryAfter.amenities ==
                  preclaimInventoryBefore.amenities - 1 &&
              preclaimInventoryAfter.chemicals ==
                  preclaimInventoryBefore.chemicals - 1,
          "physical pickup did not atomically claim canonical room supplies");
  const auto preclaimState = preclaimPersistence.save();
  auto preclaimRestored = Simulation::load(preclaimState);
  require(preclaimRestored.save() == preclaimState,
          "physical room-supply preclaim did not round-trip");
  preclaimPersistence.step(600);
  preclaimRestored.step(600);
  require(preclaimPersistence.save() == preclaimRestored.save(),
          "room-supply preclaim replayed or diverged after save/load");

  auto laborGatedClean = Simulation::tutorial(329);
  const auto laborRoom = laborGatedClean.view().rooms.front().id;
  EntityId housekeeperId = 0;
  for (const auto &person : laborGatedClean.view().people)
    if (person.kind == PersonKind::Housekeeper) {
      housekeeperId = person.id;
      break;
    }
  require(housekeeperId != 0 && laborGatedClean.fireStaff(housekeeperId).ok,
          "labor-gated housekeeping fixture could not remove housekeeper");
  const auto cleanInventoryBefore = laborGatedClean.logisticsSnapshot();
  require(laborGatedClean.requestClean(laborRoom).ok,
          "labor-gated clean request was rejected");
  const auto cleanJobBefore = laborGatedClean.housekeepingSnapshot().jobs.back();
  laborGatedClean.step(600);
  const auto cleanJobPaused = laborGatedClean.housekeepingSnapshot().jobs.back();
  require(cleanJobPaused.stage == cleanJobBefore.stage &&
              cleanJobPaused.remainingSeconds == cleanJobBefore.remainingSeconds,
          "mirrored housekeeping advanced without physical labor");
  const auto cleanInventoryPaused = laborGatedClean.logisticsSnapshot();
  const auto itemTotal = [](const LogisticsSnapshot &snapshot,
                            const char *item) {
    int total = 0;
    for (const auto &stack : snapshot.inventory)
      if (stack.item == item)
        total += stack.quantity;
    return total;
  };
  require(itemTotal(cleanInventoryPaused, "clean_linen_set") ==
              itemTotal(cleanInventoryBefore, "clean_linen_set") &&
              itemTotal(cleanInventoryPaused, "towel_unit") ==
                  itemTotal(cleanInventoryBefore, "towel_unit") &&
              itemTotal(cleanInventoryPaused, "amenity_kit") ==
                  itemTotal(cleanInventoryBefore, "amenity_kit") &&
              itemTotal(cleanInventoryPaused, "cleaning_chemical") ==
                  itemTotal(cleanInventoryBefore, "cleaning_chemical"),
          "paused mirrored housekeeping consumed canonical supplies");
  require(laborGatedClean
              .hireStaff({"Relief", PersonKind::Housekeeper, 0, 0, 18})
              .ok,
          "labor-gated housekeeping fixture could not hire relief worker");
  laborGatedClean.step(900);
  const auto cleanJobWorking = laborGatedClean.housekeepingSnapshot().jobs.back();
  require(cleanJobWorking.stage != cleanJobBefore.stage ||
              cleanJobWorking.remainingSeconds < cleanJobBefore.remainingSeconds,
          "mirrored housekeeping did not resume when physical labor started");

  auto mirroredRepair = Simulation::tutorial(327);
  const auto repairRoom = mirroredRepair.view().rooms.front().id;
  const auto engineeringBefore =
      mirroredRepair.engineeringSnapshot().workOrders.size();
  require(mirroredRepair.requestRepair(repairRoom).ok,
          "physical repair request was rejected");
  const auto mirroredEngineering = mirroredRepair.engineeringSnapshot();
  require(mirroredEngineering.workOrders.size() == engineeringBefore + 1,
          "physical repair request did not create a FINAL-04 work order");
  require(mirroredEngineering.workOrders.back().assetId == repairRoom &&
              mirroredEngineering.workOrders.back().type ==
                  WorkOrderType::Corrective,
          "mirrored FINAL-04 repair used the wrong asset or work-order type");

  auto laborGatedRepair = Simulation::tutorial(330);
  const auto laborRepairRoom = laborGatedRepair.view().rooms.front().id;
  EntityId maintenanceId = 0;
  for (const auto &person : laborGatedRepair.view().people)
    if (person.kind == PersonKind::Maintenance) {
      maintenanceId = person.id;
      break;
    }
  require(maintenanceId != 0 &&
              laborGatedRepair.fireStaff(maintenanceId).ok,
          "labor-gated engineering fixture could not remove maintenance worker");
  const auto partsBefore =
      itemTotal(laborGatedRepair.logisticsSnapshot(), "maintenance_part");
  require(laborGatedRepair.requestRepair(laborRepairRoom).ok,
          "labor-gated repair request was rejected");
  const auto repairBefore =
      laborGatedRepair.engineeringSnapshot().workOrders.back();
  laborGatedRepair.step(600);
  const auto repairPaused =
      laborGatedRepair.engineeringSnapshot().workOrders.back();
  require(repairPaused.stage == WorkOrderStage::Queued &&
              repairPaused.remainingSeconds == repairBefore.remainingSeconds,
          "mirrored engineering advanced without physical labor");
  require(itemTotal(laborGatedRepair.logisticsSnapshot(), "maintenance_part") ==
              partsBefore,
          "paused mirrored engineering consumed a canonical maintenance part");
  require(laborGatedRepair
              .hireStaff({"Relief Tech", PersonKind::Maintenance, 0, 0, 25})
              .ok,
          "labor-gated engineering fixture could not hire relief worker");
  laborGatedRepair.step(1800);
  const auto repairWorking =
      laborGatedRepair.engineeringSnapshot().workOrders.back();
  require(repairWorking.stage != WorkOrderStage::Queued ||
              repairWorking.remainingSeconds < repairBefore.remainingSeconds,
          "mirrored engineering did not resume when physical labor started");

  auto sellabilityGate = Simulation::tutorial(331);
  require(sellabilityGate
              .loadDefinitions(
                  R"({"baseDemand":0,"initialLinen":1,"initialTowels":2,"initialAmenities":1,"initialChemicals":1})")
              .ok,
          "sellability-gate inventory definitions rejected");
  const auto sellabilityRooms = sellabilityGate.view().rooms;
  require(sellabilityRooms.size() >= 2,
          "sellability-gate fixture requires two rooms");
  require(sellabilityGate.requestRoomTurn(sellabilityRooms[0].id) != 0,
          "sellability-gate fixture could not drain canonical room supplies");
  sellabilityGate.step(2400);
  require(itemTotal(sellabilityGate.logisticsSnapshot(), "clean_linen_set") == 0 &&
              itemTotal(sellabilityGate.logisticsSnapshot(), "towel_unit") == 0 &&
              itemTotal(sellabilityGate.logisticsSnapshot(), "amenity_kit") == 0 &&
              itemTotal(sellabilityGate.logisticsSnapshot(),
                        "cleaning_chemical") == 0,
          "standalone FINAL-04 turn did not drain canonical room supplies");
  require(sellabilityGate.requestClean(sellabilityRooms[1].id).ok,
          "sellability-gate physical clean request was rejected");
  sellabilityGate.step(3000);
  const auto gatedAfterPhysical = sellabilityGate.view();
  const auto gatedRoomAfterPhysical = std::find_if(
      gatedAfterPhysical.rooms.begin(), gatedAfterPhysical.rooms.end(),
      [&](const auto &room) { return room.id == sellabilityRooms[1].id; });
  require(gatedRoomAfterPhysical != gatedAfterPhysical.rooms.end(),
          "sellability-gate room disappeared");
  require(gatedRoomAfterPhysical->status == RoomStatus::VacantDirty,
          "room left dirty state before canonical supplies were claimable");
  const auto gatedPhysicalTask = std::find_if(
      gatedAfterPhysical.tasks.begin(), gatedAfterPhysical.tasks.end(),
      [&](const auto &task) {
        return task.targetId == sellabilityRooms[1].id &&
               task.kind == TaskKind::Turnover &&
               task.status != TaskStatus::Completed;
      });
  require(gatedPhysicalTask != gatedAfterPhysical.tasks.end() &&
              gatedPhysicalTask->status == TaskStatus::Blocked,
          "physical turnover was not blocked by canonical FINAL-04 stock");
  require(gatedPhysicalTask->blockedReason ==
              "Required local supplies unavailable",
          "canonical shortage did not propagate to physical task diagnostics");
  const auto gatedHousekeeping = sellabilityGate.housekeepingSnapshot();
  const auto gatedServiceJob = std::find_if(
      gatedHousekeeping.jobs.begin(), gatedHousekeeping.jobs.end(),
      [&](const auto &job) {
        return job.roomId == sellabilityRooms[1].id &&
               job.stage != HousekeepingStage::Completed;
      });
  require(gatedServiceJob != gatedHousekeeping.jobs.end(),
          "pending FINAL-04 housekeeping job disappeared while pickup was blocked");
  require(sellabilityGate.orderSupplies({1, 2, 1, 1, 0}).ok,
          "sellability-gate replenishment order was rejected");
  sellabilityGate.step(2 * 86400);
  sellabilityGate.step(3600);
  const auto gatedRecovered = sellabilityGate.view();
  const auto gatedRoomRecovered = std::find_if(
      gatedRecovered.rooms.begin(), gatedRecovered.rooms.end(),
      [&](const auto &room) { return room.id == sellabilityRooms[1].id; });
  require(gatedRoomRecovered != gatedRecovered.rooms.end() &&
              gatedRoomRecovered->status == RoomStatus::VacantReady,
          "room did not become sellable after physical and FINAL-04 work both completed");

  auto repairSellabilityGate = Simulation::tutorial(332);
  require(repairSellabilityGate
              .loadDefinitions(R"({"baseDemand":0,"initialParts":1})")
              .ok,
          "repair sellability-gate inventory definitions rejected");
  const auto repairGateRooms = repairSellabilityGate.view().rooms;
  require(repairGateRooms.size() >= 2,
          "repair sellability-gate fixture requires two rooms");
  require(repairSellabilityGate.createWorkOrder(
              repairGateRooms[0].id, WorkOrderType::Preventive) != 0,
          "repair sellability-gate could not drain canonical part");
  repairSellabilityGate.step(1000);
  require(itemTotal(repairSellabilityGate.logisticsSnapshot(),
                    "maintenance_part") == 0,
          "standalone engineering work did not drain canonical maintenance part");
  require(repairSellabilityGate
              .hireStaff({"Always Tech", PersonKind::Maintenance, 0, 0, 25})
              .ok,
          "repair sellability-gate could not hire always-on technician");
  require(repairSellabilityGate.requestRepair(repairGateRooms[1].id).ok,
          "repair sellability-gate physical repair request was rejected");
  repairSellabilityGate.step(2600);
  const auto repairBlockedView = repairSellabilityGate.view();
  const auto repairBlockedRoom = std::find_if(
      repairBlockedView.rooms.begin(), repairBlockedView.rooms.end(),
      [&](const auto &room) { return room.id == repairGateRooms[1].id; });
  require(repairBlockedRoom != repairBlockedView.rooms.end() &&
              repairBlockedRoom->status == RoomStatus::OutOfOrder,
          "room left out-of-service state before FINAL-04 repair completed");
  const auto repairEngineeringBlocked =
      repairSellabilityGate.engineeringSnapshot();
  const auto repairOrderBlocked = std::find_if(
      repairEngineeringBlocked.workOrders.begin(),
      repairEngineeringBlocked.workOrders.end(),
      [&](const auto &order) {
        return order.assetId == repairGateRooms[1].id &&
               order.type == WorkOrderType::Corrective;
      });
  require(repairOrderBlocked != repairEngineeringBlocked.workOrders.end() &&
              repairOrderBlocked->stage != WorkOrderStage::Completed,
          "canonical repair unexpectedly completed without a maintenance part");
  require(repairSellabilityGate.orderSupplies({0, 0, 0, 0, 1}).ok,
          "repair sellability-gate replenishment order was rejected");
  repairSellabilityGate.step(2 * 86400);
  repairSellabilityGate.step(3600);
  const auto repairRecoveredView = repairSellabilityGate.view();
  bool postRepairTurnover = false;
  for (const auto &task : repairRecoveredView.tasks)
    postRepairTurnover |=
        task.targetId == repairGateRooms[1].id &&
        task.kind == TaskKind::Turnover;
  require(postRepairTurnover,
          "completed FINAL-04 repair did not create the post-repair turnover");

  auto automaticTurn = Simulation::tutorial(328);
  require(automaticTurn.loadDefinitions(R"({"baseDemand":100})").ok,
          "automatic-turn definitions rejected");
  automaticTurn.step(3600);
  const auto booked = automaticTurn.view();
  require(!booked.reservations.empty(),
          "automatic-turn fixture did not create a reservation");
  int firstDeparture = booked.reservations.front().departureDay;
  for (const auto &reservation : booked.reservations)
    firstDeparture = std::min(firstDeparture, reservation.departureDay);
  const auto departureBoundary =
      static_cast<std::int64_t>(firstDeparture) * 86400 + 11 * 3600;
  require(departureBoundary > booked.elapsedSeconds,
          "automatic-turn departure boundary was not in the future");
  automaticTurn.step(
      static_cast<double>(departureBoundary - booked.elapsedSeconds));
  const auto departureView = automaticTurn.view();
  bool mirroredAutomaticTurn = false;
  for (const auto &task : departureView.tasks) {
    if (task.kind != TaskKind::Turnover ||
        task.status == TaskStatus::Completed)
      continue;
    for (const auto &job : automaticTurn.housekeepingSnapshot().jobs)
      if (job.roomId == task.targetId &&
          job.stage != HousekeepingStage::Completed)
        mirroredAutomaticTurn = true;
  }
  require(mirroredAutomaticTurn,
          "automatic checkout turnover was not mirrored into FINAL-04");

  auto shadowIgnored = Simulation::tutorial(335);
  const auto shadowIgnoredState =
      replaceLegacyInventoryLine(shadowIgnored.save(), "0 0 0 0 0");
  auto shadowIgnoredRestored = Simulation::load(shadowIgnoredState);
  const auto canonicalShadowView = shadowIgnoredRestored.view().inventory;
  require(canonicalShadowView.linen == 24 &&
              canonicalShadowView.towels == 48 &&
              canonicalShadowView.amenities == 36 &&
              canonicalShadowView.chemicals == 24 &&
              canonicalShadowView.parts == 8,
          "public inventory leaked corrupted legacy shadow state");
  const auto shadowRoom = shadowIgnoredRestored.view().rooms.front().id;
  require(shadowIgnoredRestored.requestClean(shadowRoom).ok,
          "canonical-stock clean request was rejected by empty legacy shadow");
  shadowIgnoredRestored.step(30);
  bool shadowTurnBlocked = false;
  for (const auto &task : shadowIgnoredRestored.view().tasks)
    if (task.targetId == shadowRoom && task.kind == TaskKind::Turnover &&
        task.status == TaskStatus::Blocked)
      shadowTurnBlocked = true;
  require(!shadowTurnBlocked,
          "physical turnover still depended on empty legacy shadow stock");

  auto canonicalEmpty = Simulation::tutorial(336);
  require(canonicalEmpty
              .loadDefinitions(
                  R"({"baseDemand":0,"initialLinen":0,"initialTowels":0,"initialAmenities":0,"initialChemicals":0,"initialParts":0})")
              .ok,
          "canonical-empty definitions rejected");
  const auto inflatedShadowState = replaceLegacyInventoryLine(
      canonicalEmpty.save(), "999 999 999 999 999");
  auto inflatedShadow = Simulation::load(inflatedShadowState);
  const auto emptyPublicInventory = inflatedShadow.view().inventory;
  require(emptyPublicInventory.linen == 0 && emptyPublicInventory.towels == 0 &&
              emptyPublicInventory.amenities == 0 &&
              emptyPublicInventory.chemicals == 0 &&
              emptyPublicInventory.parts == 0,
          "inflated legacy shadow leaked into canonical public stock");
  const auto emptyRoom = inflatedShadow.view().rooms.front().id;
  require(inflatedShadow.requestClean(emptyRoom).ok,
          "canonical-empty clean request could not create pending work");
  inflatedShadow.step(30);
  bool canonicalShortageBlocked = false;
  for (const auto &task : inflatedShadow.view().tasks)
    if (task.targetId == emptyRoom && task.kind == TaskKind::Turnover &&
        task.status == TaskStatus::Blocked)
      canonicalShortageBlocked = true;
  require(canonicalShortageBlocked,
          "inflated legacy shadow bypassed canonical room-supply shortage");

  auto purchasing = Simulation::tutorial(324);
  const auto purchaseCashBefore = purchasing.view().economy.cashCents;
  const auto purchase = purchasing.orderSupplies({2, 4, 2, 2, 1});
  require(purchase.ok, "player supply order was rejected");
  const auto purchaseLogistics = purchasing.logisticsSnapshot();
  require(purchaseLogistics.purchaseOrders.size() == 5,
          "player supply order did not mirror all FINAL-04 item lines");
  bool linenOrder = false;
  bool towelOrder = false;
  bool amenityOrder = false;
  bool chemicalOrder = false;
  bool partOrder = false;
  for (const auto &line : purchaseLogistics.purchaseOrders) {
    linenOrder |= line.item == "clean_linen_set" && line.quantity == 2;
    towelOrder |= line.item == "towel_unit" && line.quantity == 4;
    amenityOrder |= line.item == "amenity_kit" && line.quantity == 2;
    chemicalOrder |= line.item == "cleaning_chemical" && line.quantity == 2;
    partOrder |= line.item == "maintenance_part" && line.quantity == 1;
  }
  require(linenOrder && towelOrder && amenityOrder && chemicalOrder && partOrder,
          "player supply order used incorrect FINAL-04 item mapping");
  require(purchasing.view().supplyOrders.size() == 1,
          "legacy aggregate order record was not retained for save compatibility");
  require(purchasing.view().economy.cashCents == purchaseCashBefore - 10000,
          "mirrored supply order charged cash more than once");

  const auto cashBeforeRejected = purchasing.view().economy.cashCents;
  const auto legacyOrdersBeforeRejected = purchasing.view().supplyOrders.size();
  const auto serviceOrdersBeforeRejected =
      purchasing.logisticsSnapshot().purchaseOrders.size();
  require(!purchasing.orderSupplies({600, 0, 0, 0, 0}).ok,
          "supply order exceeding FINAL-04 storage capacity was accepted");
  require(purchasing.view().economy.cashCents == cashBeforeRejected &&
              purchasing.view().supplyOrders.size() ==
                  legacyOrdersBeforeRejected &&
              purchasing.logisticsSnapshot().purchaseOrders.size() ==
                  serviceOrdersBeforeRejected,
          "rejected mirrored supply order mutated one authority");

  const auto purchasingState = purchasing.save();
  auto purchasingRestored = Simulation::load(purchasingState);
  require(purchasingRestored.logisticsSnapshot().purchaseOrders.size() ==
              purchaseLogistics.purchaseOrders.size(),
          "mirrored FINAL-04 purchase orders did not survive save/load");

  auto wearAuthority = Simulation::tutorial(333);
  require(wearAuthority
              .loadDefinitions(
                  R"({"baseDemand":0,"roomConditionLossPerDay":1})")
              .ok,
          "wear-authority definitions rejected");
  wearAuthority.step(3 * 86400);
  const auto wearView = wearAuthority.view();
  const auto wearEngineering = wearAuthority.engineeringSnapshot();
  require(wearEngineering.failures == 0,
          "integrated FINAL-04 engineering generated a duplicate reliability failure");
  for (const auto &roomView : wearView.rooms) {
    const auto asset = std::find_if(
        wearEngineering.assets.begin(), wearEngineering.assets.end(),
        [&](const auto &candidate) { return candidate.id == roomView.id; });
    require(asset != wearEngineering.assets.end(),
            "integrated room lost its FINAL-04 engineering asset");
    require(asset->condition ==
                std::clamp(
                    static_cast<int>(std::llround(roomView.condition * 100.0)),
                    0, 10000),
            "FINAL-04 asset condition drifted from authoritative room wear");
  }

  auto definitionInventory = Simulation::tutorial(325);
  require(definitionInventory
              .loadDefinitions(
                  R"({"initialLinen":7,"initialTowels":9,"initialAmenities":5,"initialChemicals":4,"initialParts":3})")
              .ok,
          "definition inventory override was rejected");
  const auto definitionLogistics = definitionInventory.logisticsSnapshot();
  const auto totalItem = [&](const char *item) {
    int total = 0;
    for (const auto &stack : definitionLogistics.inventory)
      if (stack.item == item)
        total += stack.quantity;
    return total;
  };
  require(totalItem("clean_linen_set") == 7 &&
              totalItem("towel_unit") == 9 &&
              totalItem("amenity_kit") == 5 &&
              totalItem("cleaning_chemical") == 4 &&
              totalItem("maintenance_part") == 3,
          "definition inventory override did not synchronize FINAL-04 stock");
  const auto legacyDefinitionInventory = definitionInventory.view().inventory;
  require(legacyDefinitionInventory.linen == 7 &&
              legacyDefinitionInventory.towels == 9 &&
              legacyDefinitionInventory.amenities == 5 &&
              legacyDefinitionInventory.chemicals == 4 &&
              legacyDefinitionInventory.parts == 3,
          "definition inventory override did not update canonical public stock");
  require(definitionInventory.loadDefinitions(R"({"initialLinen":600})").ok,
          "large scenario inventory did not provision overflow storage");
  int expandedLinen = 0;
  for (const auto &stack : definitionInventory.logisticsSnapshot().inventory)
    if (stack.item == "clean_linen_set")
      expandedLinen += stack.quantity;
  require(expandedLinen == 600,
          "large scenario inventory was clipped instead of synchronized");

  const auto beforeInvalidInventory = definitionInventory.save();
  require(!definitionInventory.loadDefinitions(R"({"initialLinen":100001})").ok,
          "out-of-range definition inventory was accepted");
  require(definitionInventory.save() == beforeInvalidInventory,
          "rejected definition inventory override mutated simulation state");

  Simulation demolition(322, 16, 10, 1);
  require(demolition.loadDefinitions(R"({"baseDemand":0})").ok,
          "demolition test definitions rejected");
  for (int x = 0; x < 10; ++x)
    require(demolition
                .buildTile({0, x, 1},
                           x == 0 ? TileKind::Entrance : TileKind::Floor)
                .ok,
            "demolition test corridor build failed");
  const auto built = demolition.buildFurnishedRoom(
      {"201", 0, 4, 2, 4, 4, {0, 4, 2}, 1, 1, 120});
  require(built.ok, "demolition test room build failed");

  require(demolition.requestRoomTurn(built.id) != 0,
          "demolition test room turn not created");
  require(!demolition.removeRoom(built.id),
          "room demolished while FINAL-04 housekeeping work was active");
  require(demolition.hireStaff({"Demo Rooms", PersonKind::Housekeeper, 0, 0, 18}).ok,\n          "demolition fixture could not hire housekeeping coverage");\n  demolition.step(2400);
  require(demolition.removeRoom(built.id).ok,
          "room could not be demolished after FINAL-04 work completed");
  require(demolition.createWorkOrder(built.id, WorkOrderType::Preventive) == 0,
          "demolished room left a ghost engineering asset");
  require(demolition.requestRoomTurn(built.id) == 0,
          "demolished room left a ghost housekeeping registration");

  const auto demolishedState = demolition.save();
  require(Simulation::load(demolishedState).save() == demolishedState,
          "demolition with retired FINAL-04 state did not round-trip");

  auto parity = Simulation::tutorial(323);
  parity.step(10);
  const auto parityState = parity.save();

  const auto clockDrift = transformFinal04(
      parityState, [](const std::string &serviceState) {
        auto service = ServiceLogisticsRuntime::load(serviceState);
        service.tickSecond();
        return service.save();
      });
  requireSimulationLoadRejected(
      clockDrift,
      "Simulation accepted FINAL-04 state on a different authoritative clock");

  const auto ghostRegistration = transformFinal04(
      parityState, [](const std::string &serviceState) {
        auto service = ServiceLogisticsRuntime::load(serviceState);
        service.registerRoom(999999);
        service.registerAsset(999999, 9000);
        return service.save();
      });
  requireSimulationLoadRejected(
      ghostRegistration,
      "Simulation accepted ghost FINAL-04 room/asset registrations");

  const auto physicalRoom = parity.view().rooms.front().id;
  const auto missingRegistration = transformFinal04(
      parityState, [physicalRoom](const std::string &serviceState) {
        auto service = ServiceLogisticsRuntime::load(serviceState);
        require(service.retireRoomAndAsset(physicalRoom),
                "could not build missing-registration corruption fixture");
        return service.save();
      });
  requireSimulationLoadRejected(
      missingRegistration,
      "Simulation accepted missing FINAL-04 room/asset registrations");
}

static void room_turn_uses_staff_execution_and_main_room_state() {
  Simulation sim(322, 24, 14, 1);
  for (int x = 0; x <= 10; ++x)
    require(sim.buildTile({0, x, 5},
                          x == 0 ? TileKind::Entrance : TileKind::Floor)
                .ok,
            "service bridge corridor build failed");
  require(sim.buildTile({0, 3, 5}, TileKind::SupplyCloset).ok,
          "service bridge supply closet build failed");
  const auto built = sim.buildFurnishedRoom(
      {"101", 0, 5, 6, 5, 5, {0, 5, 6}, 1, 1, 120});
  require(built.ok, "service bridge room build failed");

  const auto turn = sim.requestRoomTurn(built.id);
  require(turn != 0, "room turn bridge did not create service job");
  require(room(sim.view(), built.id).status != RoomStatus::VacantReady,
          "room remained sellable after a FINAL-04 turn request");

  sim.step(3600);
  require(room(sim.view(), built.id).status != RoomStatus::VacantReady,
          "room turn completed without a housekeeper executing it");

  require(sim.hireStaff({"Rooms", PersonKind::Housekeeper, 0, 0, 18}).ok,
          "service bridge housekeeper hire failed");
  sim.step(5000);
  require(room(sim.view(), built.id).status == RoomStatus::VacantReady,
          "staff-executed FINAL-04 room turn never returned room to sellable");
}

static void preventive_maintenance_requires_technician_execution() {
  Simulation sim(325, 24, 14, 1);
  for (int x = 0; x <= 10; ++x)
    require(sim.buildTile({0, x, 5},
                          x == 0 ? TileKind::Entrance : TileKind::Floor)
                .ok,
            "preventive bridge corridor build failed");
  const auto built = sim.buildFurnishedRoom(
      {"201", 0, 5, 6, 5, 5, {0, 5, 6}, 1, 1, 120});
  require(built.ok, "preventive bridge room build failed");

  const auto work = sim.createWorkOrder(built.id, WorkOrderType::Preventive);
  require(work != 0, "preventive work order bridge rejected");
  const auto initial = serviceRuntime(sim).engineering().snapshot();
  const auto initialRemaining = workOrder(initial, work).remainingSeconds;
  require(initialRemaining > 0, "preventive work order started complete");

  sim.step(1800);
  const auto unattended = serviceRuntime(sim).engineering().snapshot();
  require(workOrder(unattended, work).remainingSeconds == initialRemaining,
          "preventive maintenance progressed without maintenance staff");
  require(workOrder(unattended, work).stage != WorkOrderStage::Completed,
          "preventive maintenance completed without maintenance staff");

  require(sim.hireStaff({"Engineering", PersonKind::Maintenance, 0, 0, 25}).ok,
          "preventive bridge maintenance hire failed");
  sim.step(1800);
  const auto staffed = serviceRuntime(sim).engineering().snapshot();
  require(workOrder(staffed, work).stage == WorkOrderStage::Completed,
          "on-shift maintenance staff never completed preventive work");
}

static void engineering_condition_is_main_room_authority() {
  Simulation sim(326, 24, 14, 1);
  for (int x = 0; x <= 10; ++x)
    require(sim.buildTile({0, x, 5},
                          x == 0 ? TileKind::Entrance : TileKind::Floor)
                .ok,
            "engineering authority corridor build failed");
  const auto built = sim.buildFurnishedRoom(
      {"301", 0, 5, 6, 5, 5, {0, 5, 6}, 1, 1, 120});
  require(built.ok, "engineering authority room build failed");

  const auto before = serviceRuntime(sim).engineering().snapshot();
  const auto initialCondition = engineeringAsset(before, built.id).condition;
  sim.step(3600);
  const auto after = serviceRuntime(sim).engineering().snapshot();
  const auto authoritative = engineeringAsset(after, built.id).condition;
  require(authoritative < initialCondition,
          "FINAL-04 engineering did not age the registered room asset");
  require(std::abs(room(sim.view(), built.id).condition -
                   authoritative / 100.0) < 1.0e-9,
          "main room condition diverged from FINAL-04 engineering authority");
}

static void definition_inventory_is_physical_authority() {
  auto sim = Simulation::tutorial(323);
  require(sim.loadDefinitions(
                 R"({"initialLinen":0,"initialTowels":0,"initialAmenities":0,"initialChemicals":0,"initialParts":0})")
              .ok,
          "physical inventory definition override rejected");
  const auto logistics = sim.logisticsSnapshot();
  require(itemTotal(logistics, "clean_linen_set") == 0,
          "definition override left FINAL-04 clean linen behind");
  require(itemTotal(logistics, "towel_unit") == 0,
          "definition override left FINAL-04 towels behind");
  require(itemTotal(logistics, "amenity_kit") == 0,
          "definition override left FINAL-04 amenities behind");
  require(itemTotal(logistics, "cleaning_chemical") == 0,
          "definition override left FINAL-04 chemicals behind");
  require(itemTotal(logistics, "maintenance_part") == 0,
          "definition override left FINAL-04 parts behind");

  const auto compatibility = sim.view().inventory;
  require(compatibility.linen == 0 && compatibility.towels == 0 &&
              compatibility.amenities == 0 && compatibility.chemicals == 0 &&
              compatibility.parts == 0,
          "legacy inventory view diverged from physical FINAL-04 inventory");
}

static void simulation_supply_orders_enter_receiving() {
  auto sim = Simulation::tutorial(324);
  const auto before = sim.logisticsSnapshot().purchaseOrders.size();
  require(sim.orderSupplies({0, 0, 3, 0, 0}).ok,
          "physical supply order rejected");
  const auto submitted = sim.logisticsSnapshot();
  require(submitted.purchaseOrders.size() == before + 1,
          "Simulation supply order bypassed FINAL-04 purchasing");

  sim.step(2 * 86400.0);
  const auto arrived = sim.logisticsSnapshot();
  require(itemAt(arrived, StorageKind::Receiving, "amenity_kit") == 3,
          "delivery did not physically arrive at receiving");
  sim.step(180.0);
  require(itemAt(sim.logisticsSnapshot(), StorageKind::Receiving,
                 "amenity_kit") == 0,
          "received stock never moved out of receiving");
}

int main() {
  stable_commands_and_save_boundary();
  room_turn_uses_staff_execution_and_main_room_state();
  preventive_maintenance_requires_technician_execution();
  engineering_condition_is_main_room_authority();
  definition_inventory_is_physical_authority();
  simulation_supply_orders_enter_receiving();
}
