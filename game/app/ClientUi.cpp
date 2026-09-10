#include "Client.h"

#include <algorithm>
#include <array>

namespace hh::client {

std::string toolName(Tool tool) {
  switch (tool) {
  case Tool::Inspect: return "Inspect";
  case Tool::Bedroom: return "Guest room";
  case Tool::Floor: return "Floor";
  case Tool::Wall: return "Wall";
  case Tool::Door: return "Door";
  case Tool::Entrance: return "Guest entrance";
  case Tool::Desk: return "Reception desk";
  case Tool::Closet: return "Supply closet";
  case Tool::Stairs: return "Stairs";
  case Tool::Erase: return "Remove tile";
  case Tool::Bathroom: return "Bathroom";
  case Tool::StaffRoom: return "Staff room";
  case Tool::Lobby: return "Lobby";
  }
  return "Inspect";
}

hh::game::CommandResult applyBuildTool(hh::game::Simulation &simulation,
                                       Tool tool,
                                       hh::game::Position position,
                                       std::size_t roomCount) {
  using namespace hh::game;
  if (tool == Tool::Bedroom) {
    RoomBlueprint room;
    room.name =
        "Room " + std::to_string((position.floor + 1) * 100 + roomCount + 1);
    room.floor = position.floor;
    room.x = position.x;
    room.y = position.y;
    room.width = 6;
    room.height = 6;
    room.door = {position.floor, position.x + 2, position.y + 5};
    room.nightlyRate = 120;
    return simulation.buildFurnishedRoom(room);
  }
  constexpr std::array<TileKind, 13> kinds = {
      TileKind::Empty,     TileKind::Empty,        TileKind::Floor,
      TileKind::Wall,      TileKind::Door,         TileKind::Entrance,
      TileKind::FrontDesk, TileKind::SupplyCloset, TileKind::Stairs,
      TileKind::Empty,     TileKind::Bathroom,     TileKind::StaffRoom,
      TileKind::Lobby};
  return simulation.buildTile(position, kinds[static_cast<std::size_t>(tool)]);
}

void Client::refreshUi() {
  if (!uiConfigured) {
    ui = hh::frontend::GameUiRuntime(
        [this](const hh::frontend::UiCommand &command) {
          return dispatchUiCommand(command);
        });
    uiConfigured = true;
  }
  GameUiBridgeContext contextView;
  contextView.simulationSpeed = speed;
  contextView.activeFloor = floor;
  contextView.cutaway = wallMode == hh::renderer::WallRenderMode::Cutaway;
  contextView.currentTool = toolName(tool);
  contextView.buildPreview = buildPreview;
  ui.update(makeGameUiSnapshotSource(simulation, contextView));
  hudModel.update(ui.snapshot().hud);
  alertCenter.ingest(ui.snapshot().alerts);
  objectiveUi.update(ui.snapshot().objectives);
}

hh::frontend::UiCommandResult
Client::dispatchUiCommand(const hh::frontend::UiCommand &command) {
  using namespace hh::frontend;

  GameUiCommandHooks hooks;
  hooks.setSimulationSpeed = [this](SimulationSpeed requested) {
    const int value = static_cast<int>(requested);
    if (value > 0)
      priorSpeed = value;
    speed = value;
    return UiCommandResult{true, {}, value == 0 ? "Simulation paused"
                                                : "Simulation speed changed"};
  };
  hooks.saveGame = [this] {
    const bool ok = save();
    return UiCommandResult{ok, ok ? std::string{} : "SAVE_FAILED",
                           ok ? "Campaign saved" : "Campaign save failed"};
  };
  hooks.loadGame = [this] {
    const bool ok = load();
    return UiCommandResult{ok, ok ? std::string{} : "LOAD_FAILED",
                           ok ? "Campaign loaded" : "Campaign load failed"};
  };
  hooks.openSettings = [this] {
    page = Page::Settings;
    tabScroll = 0;
    return UiCommandResult{true, {}, {}};
  };
  hooks.openInspector = [this](EntityId id) {
    selected = id;
    ui.openInspector(id);
    const auto found = std::find_if(
        ui.snapshot().entities.begin(), ui.snapshot().entities.end(),
        [id](const UiEntitySnapshot &entity) { return entity.id == id; });
    if (found == ui.snapshot().entities.end())
      return;
    switch (found->kind) {
    case InspectorKind::Room: page = Page::Rooms; break;
    case InspectorKind::Guest: page = Page::Guests; break;
    case InspectorKind::Employee:
    case InspectorKind::Department: page = Page::Staff; break;
    case InspectorKind::Inventory: page = Page::Supplies; break;
    case InspectorKind::Task:
    case InspectorKind::ServiceVenue:
    case InspectorKind::BuildJob:
    case InspectorKind::Elevator:
    case InspectorKind::BuildingSystem: page = Page::Operations; break;
    }
    tabScroll = 0;
  };
  hooks.openManagementPanel = [this](ManagementPanelId panel) {
    ui.openManagementPanel(panel);
    switch (panel) {
    case ManagementPanelId::None: break;
    case ManagementPanelId::Operations: page = Page::Operations; break;
    case ManagementPanelId::Finance: page = Page::Finance; break;
    case ManagementPanelId::Alerts: page = Page::Alerts; break;
    case ManagementPanelId::Objectives: page = Page::Objectives; break;
    case ManagementPanelId::Build: page = Page::Build; break;
    case ManagementPanelId::Rooms: page = Page::Rooms; break;
    case ManagementPanelId::Guests: page = Page::Guests; break;
    case ManagementPanelId::Staff: page = Page::Staff; break;
    case ManagementPanelId::Supplies: page = Page::Supplies; break;
    }
    tabScroll = 0;
  };
  hooks.setOverlay = [this](OverlayId requested) {
    if (requested != OverlayId::None) {
      const auto available = std::find_if(
          ui.snapshot().overlays.begin(), ui.snapshot().overlays.end(),
          [requested](const OverlaySnapshot &snapshotView) {
            return snapshotView.id == requested;
          });
      if (available == ui.snapshot().overlays.end()) {
        return UiCommandResult{false, "OVERLAY_DATA_UNAVAILABLE",
                               "This overlay is waiting for an authoritative simulation snapshot"};
      }
    }

    Overlay rendererOverlay = Overlay::Natural;
    switch (requested) {
    case OverlayId::None:
      rendererOverlay = Overlay::Natural;
      break;
    case OverlayId::Cleanliness:
      rendererOverlay = Overlay::Cleanliness;
      break;
    case OverlayId::RoomStatus:
      rendererOverlay = Overlay::Status;
      break;
    case OverlayId::RoomQuality:
    case OverlayId::MaintenanceCondition:
      rendererOverlay = Overlay::Condition;
      break;
    case OverlayId::GuestSatisfaction:
    case OverlayId::GuestTraffic:
    case OverlayId::StaffTraffic:
    case OverlayId::Noise:
    case OverlayId::Temperature:
    case OverlayId::ElectricalLoad:
    case OverlayId::WaterDemand:
    case OverlayId::OpenTaskDensity:
    case OverlayId::StaffUtilization:
    case OverlayId::QueueWait:
    case OverlayId::ElevatorCongestion:
    case OverlayId::FireSafety:
    case OverlayId::SecurityCoverage:
    case OverlayId::Revenue:
      return UiCommandResult{false, "OVERLAY_RENDERER_UNAVAILABLE",
                             "Authoritative values exist only where exposed; this world renderer has no binding for the selected overlay yet"};
    }

    managementOverlay = requested;
    ui.setOverlay(requested);
    overlay = rendererOverlay;
    return UiCommandResult{true, {},
                           requested == OverlayId::None ? "Natural view"
                                                        : "Overlay selected"};
  };
  hooks.authorityCommand = [this](const UiCommand &authority) {
    if (authority.type == UiCommandType::BuildCancel) {
      buildPreview = {};
      previewValid = false;
      tool = Tool::Inspect;
      return UiCommandResult{true, {}, "Build cancelled"};
    }
    if (authority.type == UiCommandType::BuildRotate) {
      return UiCommandResult{false, "BUILD_ROTATION_UNAVAILABLE",
                             "The current authoritative construction baseline does not expose rotation for this tool"};
    }
    if (authority.type == UiCommandType::BuildConfirm) {
      if (!buildPreview.valid || authority.integerValue !=
                                     static_cast<std::int64_t>(buildPreview.requestId)) {
        return UiCommandResult{false, "BUILD_PREVIEW_STALE",
                               "Placement must be revalidated before construction"};
      }
      const hh::game::Position position{buildPreview.floor, buildPreview.x,
                                        buildPreview.y};
      const auto out =
          applyBuildTool(simulation, tool, position, snapshot.rooms.size());
      if (out.ok && tool == Tool::Bedroom)
        selected = out.id;
      return UiCommandResult{out.ok, out.ok ? std::string{} : "BUILD_REJECTED",
                             out.message};
    }
    return UiCommandResult{
        false, "FINAL06_COMMAND_DISCONNECTED",
        "This FINAL-06 command is not attached to the authoritative Simulation object on the current integration branch"};
  };

  return routeGameUiCommand(command, hooks);
}

} // namespace hh::client
