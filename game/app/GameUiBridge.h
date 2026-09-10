#pragma once

#include "hh/frontend/GameUiTypes.h"
#include "hh/game/Simulation.h"
#include <string>

namespace hh::game {
class EconomyRuntime;
}

namespace hh::client {

struct GameUiBridgeContext {
  int simulationSpeed{};
  int activeFloor{};
  bool cutaway{true};
  std::string currentTool{"Inspect"};
  hh::frontend::BuildPlacementPreview buildPreview;
  const hh::game::EconomyRuntime* economy{};
};

[[nodiscard]] hh::frontend::SimulationSnapshot
makeGameUiSnapshotSource(const hh::game::Simulation& simulation,
                         const GameUiBridgeContext& context);

} // namespace hh::client
