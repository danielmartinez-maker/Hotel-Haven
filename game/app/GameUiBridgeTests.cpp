#include "GameUiBridge.h"
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
    const auto before = simulation.save();
    hh::client::GameUiBridgeContext context;
    context.simulationSpeed = 0;
    context.activeFloor = 0;
    context.cutaway = true;
    context.currentTool = "Inspect";
    const auto source = hh::client::makeGameUiSnapshotSource(simulation, context);
    const auto view = simulation.view();
    require(source.hud.cashCents == view.economy.cashCents, "HUD cash was not snapshot-bound");
    require(source.hud.day == view.day && source.hud.hour == view.hour, "HUD clock was not snapshot-bound");
    require(source.hud.speed == hh::frontend::SimulationSpeed::Paused, "pause speed mapping failed");
    require(source.entities.size() >= view.rooms.size(), "room inspectors were not composed");
    const auto cleanliness = std::find_if(source.overlays.begin(), source.overlays.end(), [](const auto& overlay) {
      return overlay.id == hh::frontend::OverlayId::Cleanliness;
    });
    require(cleanliness != source.overlays.end(), "cleanliness overlay was not bound");
    const auto temperature = std::find_if(source.overlays.begin(), source.overlays.end(), [](const auto& overlay) {
      return overlay.id == hh::frontend::OverlayId::Temperature;
    });
    require(temperature == source.overlays.end(), "bridge fabricated unsupported temperature authority");

    auto alternateContext = context;
    alternateContext.cutaway = false;
    const auto alternateSource =
        hh::client::makeGameUiSnapshotSource(simulation, alternateContext);
    require(alternateSource.hud.cutaway != source.hud.cutaway,
            "bridge did not copy cutaway presentation state");
    require(alternateSource.revision != source.revision,
            "cutaway presentation changes must invalidate the cached UI snapshot");

    require(simulation.save() == before, "building UI snapshot mutated simulation state");
    std::cout << "FINAL-07 game UI bridge passed\n";
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
