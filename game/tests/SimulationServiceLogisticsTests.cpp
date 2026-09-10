#include "hh/game/Simulation.h"
#include <stdexcept>
#include <string>
#include <string_view>

using namespace hh::game;

static void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

static const RoomView &room(const SimulationView &view, EntityId id) {
  for (const auto &entry : view.rooms)
    if (entry.id == id)
      return entry;
  throw std::runtime_error("room missing");
}

static int itemTotal(const LogisticsSnapshot &snapshot, std::string_view item) {
  int total = 0;
  for (const auto &stack : snapshot.inventory)
    if (stack.item == item)
      total += stack.quantity;
  return total;
}

static int itemAt(const LogisticsSnapshot &snapshot, StorageKind kind,
                  std::string_view item) {
  StorageNodeId storage{};
  for (const auto &node : snapshot.storage)
    if (node.kind == kind) {
      storage = node.id;
      break;
    }
  int total = 0;
  for (const auto &stack : snapshot.inventory)
    if (stack.storage == storage && stack.item == item)
      total += stack.quantity;
  return total;
}

static ServiceLogisticsRuntime serviceRuntime(const Simulation &sim) {
  const auto encoded = sim.save();
  const auto marker = encoded.rfind("FINAL04 ");
  require(marker != std::string::npos, "Simulation save omitted FINAL-04 section");
  const auto lineEnd = encoded.find('\n', marker);
  require(lineEnd != std::string::npos, "FINAL-04 section header is malformed");
  const auto byteText = encoded.substr(marker + 8, lineEnd - (marker + 8));
  const auto bytes = static_cast<std::size_t>(std::stoull(byteText));
  const auto payloadStart = lineEnd + 1;
  require(payloadStart + bytes <= encoded.size(),
          "FINAL-04 section length exceeds Simulation save");
  return ServiceLogisticsRuntime::load(encoded.substr(payloadStart, bytes));
}

static const WorkOrderView &workOrder(const EngineeringSnapshot &snapshot,
                                      WorkOrderId id) {
  for (const auto &entry : snapshot.workOrders)
    if (entry.id == id)
      return entry;
  throw std::runtime_error("engineering work order missing");
}

static void stable_commands_and_save_boundary() {
  auto sim = Simulation::tutorial(321);
  const auto view = sim.view();
  require(!view.rooms.empty(), "tutorial requires a serviceable room");
  const auto roomId = view.rooms.front().id;

  const auto turn = sim.requestRoomTurn(roomId);
  require(turn != 0, "Simulation did not expose room-turn command");

  const auto work = sim.createWorkOrder(roomId, WorkOrderType::Preventive);
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
  definition_inventory_is_physical_authority();
  simulation_supply_orders_enter_receiving();
}
