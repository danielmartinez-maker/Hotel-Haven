#pragma once
#include "hh/game/BuildingSystems.h"
#include "hh/game/BuildJobs.h"
#include "hh/game/Construction.h"
#include "hh/game/Simulation.h"
#include "hh/renderer/RenderScene.h"
namespace hh::client {
enum class Overlay { Natural, Status, Cleanliness, Condition, Utilities, Egress };
struct WorldViewOptions {
  int floor{};
  hh::game::EntityId selected{};
  Overlay overlay{Overlay::Natural};
  int hoverX{-1}, hoverY{-1};
  float previewSize{1};
  bool showPreview{}, previewValid{};
  const hh::game::ConstructionSnapshot *construction{};
  const hh::game::BuildingSystemsSnapshot *buildingSystems{};
};
hh::renderer::RenderScene worldScene(const hh::game::SimulationView &,
                                     const WorldViewOptions &);
hh::renderer::RenderScene
worldSceneWithSystems(const hh::game::SimulationView &,
                      const WorldViewOptions &);
} // namespace hh::client
