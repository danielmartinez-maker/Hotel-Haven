#pragma once

#include "hh/frontend/GameUiTypes.h"
#include <functional>

namespace hh::client {

struct GameUiCommandHooks {
  std::function<hh::frontend::UiCommandResult(hh::frontend::SimulationSpeed)>
      setSimulationSpeed;
  std::function<hh::frontend::UiCommandResult()> saveGame;
  std::function<hh::frontend::UiCommandResult()> loadGame;
  std::function<hh::frontend::UiCommandResult()> openSettings;
  std::function<void(hh::frontend::EntityId)> openInspector;
  std::function<void(hh::frontend::ManagementPanelId)> openManagementPanel;
  std::function<hh::frontend::UiCommandResult(hh::frontend::OverlayId)>
      setOverlay;
  std::function<hh::frontend::UiCommandResult(const hh::frontend::UiCommand&)>
      authorityCommand;
};

[[nodiscard]] hh::frontend::UiCommandResult
routeGameUiCommand(const hh::frontend::UiCommand& command,
                   const GameUiCommandHooks& hooks);

} // namespace hh::client
