#pragma once
#include "hh/game/Simulation.h"
#include "hh/renderer/RenderScene.h"
namespace hh::client {
enum class Overlay { Natural, Status, Cleanliness, Condition };
struct WorldViewOptions {
  int floor{};
  hh::game::EntityId selected{};
  Overlay overlay{Overlay::Natural};
  int hoverX{-1}, hoverY{-1};
  float previewSize{1};
  bool showPreview{}, previewValid{};
};
hh::renderer::RenderScene worldScene(const hh::game::SimulationView &,
                                     const WorldViewOptions &);
} // namespace hh::client
