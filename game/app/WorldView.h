#pragma once
#include "hh/game/Simulation.h"
#include "hh/renderer/RenderScene.h"
#include <optional>

namespace hh::client {
enum class Overlay { Natural, Status, Cleanliness, Condition };

struct WorldAssetVisual {
  hh::renderer::AssetHandle handle;
  hh::renderer::Aabb localBounds;
  bool translucent{};
};

// Resolved once at client startup. WorldView consumes only compact renderer
// handles and local bounds; it never performs filesystem or string-ID lookup.
struct WorldAssetSet {
  std::optional<WorldAssetVisual> straightStair;
  std::optional<WorldAssetVisual> guestBed;
  std::optional<WorldAssetVisual> nightstand;
  std::optional<WorldAssetVisual> guestDesk;
  std::optional<WorldAssetVisual> bathroomVanity;
  std::optional<WorldAssetVisual> bathroomToilet;
  std::optional<WorldAssetVisual> showerGlass;
  std::optional<WorldAssetVisual> receptionDesk;
  std::optional<WorldAssetVisual> pottedPlant;
};

struct WorldViewOptions {
  int floor{};
  hh::game::EntityId selected{};
  Overlay overlay{Overlay::Natural};
  int hoverX{-1}, hoverY{-1};
  float previewSize{1};
  bool showPreview{}, previewValid{};
};

hh::renderer::RenderScene worldScene(
    const hh::game::SimulationView &,
    const WorldViewOptions &,
    const WorldAssetSet *assets = nullptr);
} // namespace hh::client
