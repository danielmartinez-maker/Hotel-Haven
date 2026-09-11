#pragma once

#include "hh/frontend/GameUiTypes.h"
#include "hh/game/SimulationEconomyBridge.h"

namespace hh::client {

[[nodiscard]] hh::frontend::UiCommandResult
dispatchFinal06UiCommand(hh::game::SimulationEconomyBridge &authority,
                         const hh::frontend::UiCommand &command);

} // namespace hh::client
