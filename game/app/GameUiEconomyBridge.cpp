#include "GameUiBridge.h"

namespace hh::client {

hh::frontend::SimulationSnapshot
makeGameUiSnapshotSource(const hh::game::SimulationEconomyBridge &simulation,
                         const GameUiBridgeContext &context) {
  auto bridgedContext = context;
  bridgedContext.economy = &simulation.economyRuntime();
  return makeGameUiSnapshotSource(simulation.physicalSimulation(), bridgedContext);
}

} // namespace hh::client
