#include "hh/game/Simulation.h"
#include <algorithm>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace hh::game;
static void require(bool value, const char *message) { if (!value) throw std::runtime_error(message); }

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

int main() {
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
  require(after - before == 10, "service/logistics clock is not driven by Simulation");

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
  const auto physicalTurnsAfterFirst = std::count_if(
      mirroredClean.view().tasks.begin(), mirroredClean.view().tasks.end(),
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
  const auto physicalTurnsAfterSecond = std::count_if(
      mirroredClean.view().tasks.begin(), mirroredClean.view().tasks.end(),
      [mirroredRoom](const auto &task) {
        return task.targetId == mirroredRoom &&
               task.kind == TaskKind::Turnover &&
               task.status != TaskStatus::Completed;
      });
  require(physicalTurnsAfterFirst == 1 && physicalTurnsAfterSecond == 1,
          "duplicate physical clean request created duplicate worker tasks");

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
          "legacy delivery bridge was not retained for the physical scheduler");
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
          "definition inventory override did not preserve legacy shadow stock");
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
  demolition.step(2400);
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
