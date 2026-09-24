#include "GameUiBridge.h"
#include "LiveBuildCatalog.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool value, const char* message) {
  if (!value) throw std::runtime_error(message);
}
}

int main() {
  try {
    hh::game::Simulation simulation = hh::game::Simulation::tutorial(20260909);
    hh::game::EntityId cleanedRoomId = 0;
    for (const auto& room : simulation.view().rooms) {
      if (simulation.requestClean(room.id).ok) {
        cleanedRoomId = room.id;
        break;
      }
    }
    require(cleanedRoomId != 0, "test fixture could not create an authoritative room task");
    const auto before = simulation.save();
    hh::client::GameUiBridgeContext context;
    context.simulationSpeed = 0;
    context.activeFloor = 0;
    context.cutaway = true;
    context.currentTool = "Inspect";
    auto source = hh::client::makeGameUiSnapshotSource(simulation, context);
    hh::client::attachLiveBuildCatalog(source);
    const auto view = simulation.view();
    require(source.hud.cashCents == view.economy.cashCents, "HUD cash was not snapshot-bound");
    require(source.hud.day == view.day && source.hud.hour == view.hour, "HUD clock was not snapshot-bound");
    require(source.hud.speed == hh::frontend::SimulationSpeed::Paused, "pause speed mapping failed");
    require(source.entities.size() >= view.rooms.size(), "room inspectors were not composed");

    hh::game::Simulation departmentSimulation =
        hh::game::Simulation::tutorial(20260913);
    hh::game::EntityId departmentManager = 0;
    for (const auto& person : departmentSimulation.view().people) {
      if (person.kind == hh::game::PersonKind::Housekeeper) {
        departmentManager = person.id;
        break;
      }
    }
    require(departmentManager != 0,
            "department UI fixture could not find a housekeeping manager");
    require(departmentSimulation
                .assignDepartmentManager(hh::game::DepartmentId::Housekeeping,
                                         departmentManager)
                .ok,
            "department UI fixture could not assign a manager");
    const auto departmentBefore = departmentSimulation.save();
    const auto departmentSource =
        hh::client::makeGameUiSnapshotSource(departmentSimulation, context);
    require(departmentSimulation.save() == departmentBefore,
            "department UI projection mutated simulation state");

    const auto departmentEntity = std::find_if(
        departmentSource.entities.begin(), departmentSource.entities.end(),
        [](const auto& entity) {
          return entity.kind == hh::frontend::InspectorKind::Department &&
                 entity.title == "Housekeeping";
        });
    require(departmentEntity != departmentSource.entities.end(),
            "housekeeping department inspector was omitted");

    const auto departmentField =
        [&](const char* label) -> std::string {
      const auto field = std::find_if(
          departmentEntity->fields.begin(), departmentEntity->fields.end(),
          [&](const auto& candidate) { return candidate.label == label; });
      require(field != departmentEntity->fields.end(),
              "department inspector field was omitted");
      return field->value;
    };
    const auto departmentForecast = departmentSimulation.departmentForecast(
        hh::game::DepartmentId::Housekeeping,
        departmentSimulation.view().day);
    require(departmentField("Manager") == std::to_string(departmentManager),
            "department inspector did not expose authoritative manager");
    require(departmentField("Required today") ==
                std::to_string(departmentForecast.requiredMinutes) + " min" &&
                departmentField("Scheduled today") ==
                    std::to_string(departmentForecast.scheduledMinutes) + " min" &&
                departmentField("Uncovered today") ==
                    std::to_string(departmentForecast.uncoveredMinutes) + " min",
            "department inspector forecast drifted from authoritative state");

    const auto managedRevision = departmentSource.revision;
    require(departmentSimulation
                .assignDepartmentManager(hh::game::DepartmentId::Housekeeping, 0)
                .ok,
            "department UI fixture could not clear manager");
    const auto unmanagedSource =
        hh::client::makeGameUiSnapshotSource(departmentSimulation, context);
    require(unmanagedSource.revision != managedRevision,
            "department manager mutation did not invalidate UI revision");

    hh::game::Simulation serviceSimulation =
        hh::game::Simulation::tutorial(20260911);
    const auto serviceRoom = serviceSimulation.view().rooms.front().id;
    const auto serviceBefore =
        hh::client::makeGameUiSnapshotSource(serviceSimulation, context);
    const auto roomTurnId = serviceSimulation.requestRoomTurn(serviceRoom);
    require(roomTurnId != 0,
            "FINAL-04 housekeeping fixture could not start a room turn");
    const auto engineeringId = serviceSimulation.createWorkOrder(
        serviceRoom, hh::game::WorkOrderType::Corrective);
    require(engineeringId != 0,
            "FINAL-04 engineering fixture could not create a work order");
    const auto serviceSource =
        hh::client::makeGameUiSnapshotSource(serviceSimulation, context);
    require(serviceSource.revision != serviceBefore.revision,
            "paused FINAL-04 service mutation did not invalidate UI revision");
    require(serviceSource.operations.housekeepingBacklog == 1,
            "housekeeping KPI did not follow FINAL-04 room-turn queue");
    require(serviceSource.operations.engineeringOpenOrders == 1,
            "engineering KPI did not follow FINAL-04 work-order queue");
    const auto serviceHousekeeping = std::find_if(
        serviceSource.operations.rows.begin(), serviceSource.operations.rows.end(),
        [&](const auto& row) {
          return row.area == hh::frontend::OperationArea::Housekeeping &&
                 row.targetId == serviceRoom &&
                 row.name == "Room turn #" + std::to_string(roomTurnId);
        });
    require(serviceHousekeeping != serviceSource.operations.rows.end(),
            "FINAL-04 housekeeping operation was omitted from dashboard");
    const auto serviceEngineering = std::find_if(
        serviceSource.operations.rows.begin(), serviceSource.operations.rows.end(),
        [&](const auto& row) {
          return row.area == hh::frontend::OperationArea::Engineering &&
                 row.targetId == serviceRoom;
        });
    require(serviceEngineering != serviceSource.operations.rows.end(),
            "FINAL-04 engineering operation was omitted from dashboard");

    hh::game::Simulation legacyOrderSimulation =
        hh::game::Simulation::tutorial(20260912);
    require(legacyOrderSimulation.orderSupplies({1, 0, 0, 0, 0}).ok,
            "legacy supply-order fixture could not submit an order");
    require(!legacyOrderSimulation.view().supplyOrders.empty(),
            "legacy supply-order fixture did not create legacy order state");
    const auto legacyOrderSource =
        hh::client::makeGameUiSnapshotSource(legacyOrderSimulation, context);
    require(legacyOrderSource.operations.incomingOrders == 1,
            "FINAL-07 operations dashboard did not project the mirrored FINAL-04 order exactly once");

    hh::game::Simulation inventorySimulation =
        hh::game::Simulation::tutorial(20260910);
    const auto canonicalInventoryBefore = inventorySimulation.view().inventory;
    const auto inventoryRoom = inventorySimulation.view().rooms.front().id;
    require(inventorySimulation.requestRoomTurn(inventoryRoom) != 0,
            "FINAL-04 inventory fixture could not start a room turn");
    inventorySimulation.step(1500);
    const auto canonicalInventoryAfter = inventorySimulation.view().inventory;
    require(canonicalInventoryAfter.linen ==
                canonicalInventoryBefore.linen - 1 &&
                canonicalInventoryAfter.towels ==
                    canonicalInventoryBefore.towels - 2 &&
                canonicalInventoryAfter.amenities ==
                    canonicalInventoryBefore.amenities - 1 &&
                canonicalInventoryAfter.chemicals ==
                    canonicalInventoryBefore.chemicals - 1,
            "SimulationView inventory did not follow canonical FINAL-04 consumption");

    const auto inventorySource =
        hh::client::makeGameUiSnapshotSource(inventorySimulation, context);
    const auto inventoryEntity = std::find_if(
        inventorySource.entities.begin(), inventorySource.entities.end(),
        [](const auto& entity) {
          return entity.kind == hh::frontend::InspectorKind::Inventory &&
                 entity.title == "Property inventory";
        });
    require(inventoryEntity != inventorySource.entities.end(),
            "authoritative property inventory inspector was omitted");

    const auto fieldValue = [&](const char* label) -> std::string {
      const auto field = std::find_if(
          inventoryEntity->fields.begin(), inventoryEntity->fields.end(),
          [&](const auto& candidate) { return candidate.label == label; });
      require(field != inventoryEntity->fields.end(),
              "authoritative inventory field was omitted");
      return field->value;
    };
    require(fieldValue("Linen") == "23" && fieldValue("Towels") == "46" &&
                fieldValue("Amenities") == "35" &&
                fieldValue("Chemicals") == "23" &&
                fieldValue("Parts") == "8",
            "property inventory inspector did not follow FINAL-04 physical stock");
    require(inventorySource.operations.cleanLinenUnits == 23,
            "operations clean-linen KPI did not use FINAL-04 clean_linen_set stock");

    const auto task = std::find_if(view.tasks.begin(), view.tasks.end(),
        [cleanedRoomId](const auto& candidate) {
          return candidate.targetId == cleanedRoomId &&
                 candidate.kind == hh::game::TaskKind::Turnover;
        });
    require(task != view.tasks.end(),
            "created room turnover task disappeared before UI projection");
    const auto operation = std::find_if(
        source.operations.rows.begin(), source.operations.rows.end(),
        [task](const auto& row) {
          return row.id == task->id &&
                 row.area == hh::frontend::OperationArea::Staffing &&
                 row.name == "Room turnover";
        });
    require(operation != source.operations.rows.end(),
            "physical workforce turnover task was omitted from operations dashboard");
    require(source.operations.housekeepingBacklog == 1,
            "mirrored room turnover was missing from FINAL-04 housekeeping backlog");
    const auto mirroredHousekeeping = std::find_if(
        source.operations.rows.begin(), source.operations.rows.end(),
        [cleanedRoomId](const auto& row) {
          return row.area == hh::frontend::OperationArea::Housekeeping &&
                 row.targetId == cleanedRoomId;
        });
    require(mirroredHousekeeping != source.operations.rows.end(),
            "physical room turnover did not expose its FINAL-04 counterpart");
    require(operation->assigneeId == task->employeeId,
            "operation assignee did not preserve authoritative employee id");
    require(operation->targetId == task->targetId,
            "operation target entity did not preserve authoritative target id");
    require(operation->targetFloor == task->target.floor &&
                operation->targetX == task->target.x &&
                operation->targetY == task->target.y,
            "operation location did not preserve authoritative task target");

    const auto cleanliness = std::find_if(source.overlays.begin(), source.overlays.end(), [](const auto& overlay) {
      return overlay.id == hh::frontend::OverlayId::Cleanliness;
    });
    require(cleanliness != source.overlays.end(), "cleanliness overlay was not bound");
    const auto temperature = std::find_if(source.overlays.begin(), source.overlays.end(), [](const auto& overlay) {
      return overlay.id == hh::frontend::OverlayId::Temperature;
    });
    require(temperature == source.overlays.end(), "bridge fabricated unsupported temperature authority");

    const auto roomQuality = std::find_if(source.overlays.begin(), source.overlays.end(), [](const auto& overlay) {
      return overlay.id == hh::frontend::OverlayId::RoomQuality;
    });
    require(roomQuality == source.overlays.end(),
            "bridge fabricated room quality from maintenance condition");
    const auto maintenance = std::find_if(source.overlays.begin(), source.overlays.end(), [](const auto& overlay) {
      return overlay.id == hh::frontend::OverlayId::MaintenanceCondition;
    });
    require(maintenance != source.overlays.end(),
            "maintenance overlay lost its authoritative room-condition source");

    require(source.buildCatalog.size() == 12,
            "live build catalog must expose every currently buildable construction tool");
    const auto guestRoom = std::find_if(
        source.buildCatalog.begin(), source.buildCatalog.end(), [](const auto& item) {
          return item.id == "Guest room";
        });
    require(guestRoom != source.buildCatalog.end(),
            "live build catalog omitted the furnished guest-room command");
    require(guestRoom->name == "Guest room" && guestRoom->category == "Rooms",
            "guest-room build metadata does not match the live construction command");
    require(guestRoom->costCents == 540000,
            "guest-room catalog cost drifted from the 6x6 furnished-room command");
    require(guestRoom->id == guestRoom->name,
            "catalog id must match the native placement-preview item id");
    const auto floorTile = std::find_if(
        source.buildCatalog.begin(), source.buildCatalog.end(), [](const auto& item) {
          return item.id == "Floor";
        });
    require(floorTile != source.buildCatalog.end() && floorTile->costCents == 500,
            "tile catalog cost drifted from the live tile-construction command");

    const auto guestRoomTool = hh::client::liveBuildTool("Guest room");
    require(guestRoomTool.has_value() && static_cast<int>(*guestRoomTool) == 1,
            "guest-room catalog id must resolve to the Bedroom construction tool");
    const auto entranceTool = hh::client::liveBuildTool("Guest entrance");
    require(entranceTool.has_value() && static_cast<int>(*entranceTool) == 5,
            "guest-entrance catalog id must resolve to the Entrance construction tool");
    for (const auto& item : source.buildCatalog) {
      require(hh::client::liveBuildTool(item.id).has_value(),
              "every visible live catalog item must resolve to an executable client tool");
    }
    require(!hh::client::liveBuildTool("unsupported_future_tool").has_value(),
            "unknown catalog ids must never resolve to an executable client tool");

    auto alternateContext = context;
    alternateContext.cutaway = false;
    const auto alternateSource =
        hh::client::makeGameUiSnapshotSource(simulation, alternateContext);
    require(alternateSource.hud.cutaway != source.hud.cutaway,
            "bridge did not copy cutaway presentation state");

    require(simulation.save() == before, "building UI snapshot mutated simulation state");
    std::cout << "FINAL-07 game UI bridge passed\n";
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
