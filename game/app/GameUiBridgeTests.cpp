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
                 row.area == hh::frontend::OperationArea::Housekeeping &&
                 row.name == "Room turnover";
        });
    require(operation != source.operations.rows.end(),
            "authoritative turnover task was omitted from operations dashboard");
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
