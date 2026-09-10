#include "GameUiCommandRouter.h"

namespace hh::client {
namespace {

using hh::frontend::ManagementPanelId;
using hh::frontend::OverlayId;
using hh::frontend::SimulationSpeed;
using hh::frontend::UiCommand;
using hh::frontend::UiCommandResult;
using hh::frontend::UiCommandType;

UiCommandResult unavailable(const char* reason, const char* message) {
  return {false, reason, message};
}

bool validSpeed(std::int64_t value) noexcept {
  return value == static_cast<std::int64_t>(SimulationSpeed::Paused) ||
         value == static_cast<std::int64_t>(SimulationSpeed::OneX) ||
         value == static_cast<std::int64_t>(SimulationSpeed::TwoX) ||
         value == static_cast<std::int64_t>(SimulationSpeed::FourX) ||
         value == static_cast<std::int64_t>(SimulationSpeed::EightX);
}

bool validPanel(std::int64_t value) noexcept {
  return value >= static_cast<std::int64_t>(ManagementPanelId::None) &&
         value <= static_cast<std::int64_t>(ManagementPanelId::Supplies);
}

bool validOverlay(std::int64_t value) noexcept {
  return value >= static_cast<std::int64_t>(OverlayId::None) &&
         value <= static_cast<std::int64_t>(OverlayId::Revenue);
}

} // namespace

hh::frontend::UiCommandResult
routeGameUiCommand(const hh::frontend::UiCommand& command,
                   const GameUiCommandHooks& hooks) {
  switch (command.type) {
  case UiCommandType::None:
    return unavailable("UI_COMMAND_EMPTY", "No UI command was supplied");

  case UiCommandType::SetSimulationSpeed:
    if (!validSpeed(command.integerValue)) {
      return unavailable("UI_SPEED_INVALID",
                         "Simulation speed must be 0x, 1x, 2x, 4x, or 8x");
    }
    if (!hooks.setSimulationSpeed) {
      return unavailable("UI_SPEED_UNAVAILABLE",
                         "Simulation speed application hook is unavailable");
    }
    return hooks.setSimulationSpeed(
        static_cast<SimulationSpeed>(command.integerValue));

  case UiCommandType::PauseGame:
    if (!hooks.setSimulationSpeed) {
      return unavailable("UI_SPEED_UNAVAILABLE",
                         "Simulation speed application hook is unavailable");
    }
    return hooks.setSimulationSpeed(SimulationSpeed::Paused);

  case UiCommandType::SaveGame:
    if (!hooks.saveGame)
      return unavailable("UI_SAVE_UNAVAILABLE", "Save application hook is unavailable");
    return hooks.saveGame();

  case UiCommandType::LoadGame:
    if (!hooks.loadGame)
      return unavailable("UI_LOAD_UNAVAILABLE", "Load application hook is unavailable");
    return hooks.loadGame();

  case UiCommandType::OpenSettings:
    if (!hooks.openSettings)
      return unavailable("UI_SETTINGS_UNAVAILABLE",
                         "Settings application hook is unavailable");
    return hooks.openSettings();

  case UiCommandType::OpenInspector:
    if (!hooks.openInspector)
      return unavailable("UI_INSPECTOR_UNAVAILABLE",
                         "Inspector navigation hook is unavailable");
    hooks.openInspector(command.entityId);
    return {true, {}, {}};

  case UiCommandType::OpenManagementPanel:
    if (!validPanel(command.integerValue))
      return unavailable("UI_PANEL_INVALID", "Unknown management panel");
    if (!hooks.openManagementPanel)
      return unavailable("UI_PANEL_UNAVAILABLE",
                         "Management panel navigation hook is unavailable");
    hooks.openManagementPanel(
        static_cast<ManagementPanelId>(command.integerValue));
    return {true, {}, {}};

  case UiCommandType::SetOverlay:
    if (!validOverlay(command.integerValue))
      return unavailable("UI_OVERLAY_INVALID", "Unknown overlay");
    if (!hooks.setOverlay)
      return unavailable("UI_OVERLAY_UNAVAILABLE",
                         "Overlay presentation hook is unavailable");
    return hooks.setOverlay(static_cast<OverlayId>(command.integerValue));

  case UiCommandType::BuildConfirm:
  case UiCommandType::BuildRotate:
  case UiCommandType::BuildCancel:
  case UiCommandType::SetFutureRate:
  case UiCommandType::SetOverbookingPolicy:
  case UiCommandType::StartMarketingCampaign:
  case UiCommandType::AcceptContract:
    if (!hooks.authorityCommand) {
      return unavailable("UI_AUTHORITY_UNAVAILABLE",
                         "Authoritative gameplay command hook is unavailable");
    }
    return hooks.authorityCommand(command);
  }

  return unavailable("UI_COMMAND_UNKNOWN", "Unknown UI command");
}

} // namespace hh::client
