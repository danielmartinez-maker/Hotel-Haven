#pragma once
#include "hh/game/Simulation.h"
#include "hh/renderer/RenderScene.h"
#include <array>
#include <cstddef>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>

namespace hh::client {
enum class Overlay {
  Natural,
  Status,
  Cleanliness,
  Condition,
  GuestSatisfaction,
  StaffUtilization,
  QueueWait,
  OpenTaskDensity
};

struct WorldAssetVisual {
  hh::renderer::AssetHandle handle;
  hh::renderer::Aabb localBounds;
  bool translucent{};
};

// Resolved once at client startup. WorldView consumes only compact renderer
// handles and local bounds; it never performs filesystem or string-ID lookup.
struct WorldAssetSet {
  static constexpr std::size_t GuestCharacterVariantCount = 30;
  static constexpr std::size_t ReceptionistVariantCount = 2;
  static constexpr std::size_t HousekeeperVariantCount = 2;
  static constexpr std::size_t MaintenanceVariantCount = 2;

  std::optional<WorldAssetVisual> straightStair;
  std::optional<WorldAssetVisual> standardGuestDoor;
  std::optional<WorldAssetVisual> lobbyEntranceDoor;
  std::optional<WorldAssetVisual> singleBed;
  std::optional<WorldAssetVisual> doubleBed;
  std::optional<WorldAssetVisual> queenBed;
  std::optional<WorldAssetVisual> kingBed;
  std::optional<WorldAssetVisual> twinBedLeft;
  std::optional<WorldAssetVisual> twinBedRight;
  std::optional<WorldAssetVisual> nightstand;
  std::optional<WorldAssetVisual> bedsideLamp;
  std::optional<WorldAssetVisual> guestDesk;
  std::optional<WorldAssetVisual> deskChair;
  std::optional<WorldAssetVisual> guestArmchair;
  std::optional<WorldAssetVisual> luggageBench;
  std::optional<WorldAssetVisual> wardrobe;
  std::optional<WorldAssetVisual> tvConsole;
  std::optional<WorldAssetVisual> wallTelevision;
  std::optional<WorldAssetVisual> bathroomVanity;
  std::optional<WorldAssetVisual> bathroomToilet;
  std::optional<WorldAssetVisual> showerGlass;
  std::optional<WorldAssetVisual> receptionDesk;
  std::optional<WorldAssetVisual> lobbySofa;
  std::optional<WorldAssetVisual> lobbyArmchair;
  std::optional<WorldAssetVisual> lobbyCoffeeTable;
  std::optional<WorldAssetVisual> lobbyFloorLamp;
  std::optional<WorldAssetVisual> lobbyPlanter;
  std::optional<WorldAssetVisual> luggageCart;
  std::optional<WorldAssetVisual> housekeepingCart;
  std::optional<WorldAssetVisual> utilityCart;
  std::optional<WorldAssetVisual> cleaningSupplyCabinet;
  std::optional<WorldAssetVisual> staffLockerBank;
  std::optional<WorldAssetVisual> staffBench;
  std::optional<WorldAssetVisual> pottedPlant;

  std::array<std::optional<WorldAssetVisual>, GuestCharacterVariantCount>
      guestCharacters{};
  std::array<std::optional<WorldAssetVisual>, ReceptionistVariantCount>
      receptionistCharacters{};
  std::array<std::optional<WorldAssetVisual>, HousekeeperVariantCount>
      housekeeperCharacters{};
  std::array<std::optional<WorldAssetVisual>, MaintenanceVariantCount>
      maintenanceCharacters{};

  // Full milestone library available to presentation code. Named members above
  // remain the hot-path bindings for common gameplay visuals; this map lets
  // V2 content scale without adding one C++ field per asset.
  std::unordered_map<std::string, WorldAssetVisual> catalog;
};

using WorldAssetResolver =
    std::function<WorldAssetVisual(std::string_view assetId)>;

// Stable shipping-client dependency set. The runtime registry uses this list to
// decode only meshes that the current world presentation can instantiate.
[[nodiscard]] std::span<const std::string_view> requiredWorldAssetIds() noexcept;

// The string IDs are an installation/startup concern only. This converts them
// once to the compact visual records consumed by every subsequent frame.
WorldAssetSet resolveWorldAssets(const WorldAssetResolver &resolver);

[[nodiscard]] const WorldAssetVisual* findWorldAsset(
    const WorldAssetSet& assets,
    std::string_view assetId) noexcept;

struct WorldViewOptions {
  int floor{};
  hh::game::EntityId selected{};
  Overlay overlay{Overlay::Natural};
  int hoverX{-1}, hoverY{-1};
  float previewSize{1};
  bool showPreview{}, previewValid{};
  bool reducedMotion{};
};

hh::renderer::RenderScene worldScene(
    const hh::game::SimulationView &,
    const WorldViewOptions &,
    const WorldAssetSet *assets = nullptr);

// Deterministic renderer smoke/gallery scene for the bounded runtime catalog.
// It has no simulation or placement authority.
hh::renderer::RenderScene runtimeAssetCatalogScene(
    const WorldAssetSet &assets,
    std::uint32_t firstAssetNumber = 1u,
    std::uint32_t lastAssetNumber = 700u);
} // namespace hh::client
