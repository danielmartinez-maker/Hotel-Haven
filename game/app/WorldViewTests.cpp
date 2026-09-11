#include "WorldView.h"
#include "hh/game/BuildingSystems.h"
#include "hh/game/Construction.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace hh::client;
using namespace hh::renderer;
static void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}
int main() {
  try {
    auto game = hh::game::Simulation::tutorial(19);
    const auto before = game.save();
    const auto snapshot = game.view();
    require(!snapshot.rooms.empty(), "starter must contain furnished rooms");
    WorldViewOptions options;
    options.selected = snapshot.rooms.front().id;
    const auto scene = worldScene(snapshot, options);
    require(scene.items.size() > snapshot.tiles.size(),
            "world omitted furnishing geometry");
    require(scene.focusTarget.has_value(),
            "selected room has no cutaway focus");
    for (const auto &item : scene.items) {
      require(std::isfinite(item.center.x) && std::isfinite(item.center.y) &&
                  std::isfinite(item.center.z),
              "non-finite geometry");
      require(item.size.x > 0 && item.size.y > 0 && item.size.z > 0,
              "non-positive box dimension");
      require(item.bounds.min.x <= item.center.x &&
                  item.bounds.max.x >= item.center.x,
              "invalid selection bounds");
    }
    options.overlay = Overlay::Cleanliness;
    const auto heatmap = worldScene(snapshot, options);
    require(heatmap.items.size() == scene.items.size(),
            "heatmap changed physical geometry");
    bool changed = false;
    for (std::size_t i = 0; i < scene.items.size(); ++i)
      changed |= scene.items[i].color.r != heatmap.items[i].color.r;
    require(changed, "cleanliness heatmap has no diagnostic colors");
    options.showPreview = true;
    options.previewSize = 6;
    options.previewValid = false;
    options.hoverX = snapshot.width - 1;
    options.hoverY = snapshot.height - 1;
    const auto ghost = worldScene(snapshot, options).items.back();
    require(ghost.size.x == 1 && ghost.size.z == 1,
            "out-of-map preview was not clipped");
    require(ghost.color.r > ghost.color.g, "invalid preview must be red");
    require(game.save() == before,
            "world rendering mutated authoritative state");

    hh::game::Simulation constructionGame(1901, 8, 8, 1);
    for (int y = 0; y < 8; ++y)
      for (int x = 0; x < 8; ++x)
        require(constructionGame
                    .buildTile({0, x, y}, x == 0 && y == 0
                                                ? hh::game::TileKind::Entrance
                                                : hh::game::TileKind::Floor)
                    .ok,
                "construction render fixture floor failed");
    hh::game::ConstructionCommand placement;
    placement.placements.push_back({"chair", {0, 3, 3}, 0});
    require(constructionGame.executeConstruction(placement).ok,
            "construction render fixture placement failed");
    const auto constructionState = constructionGame.constructionSnapshot();
    WorldViewOptions constructionOptions;
    constructionOptions.construction = &constructionState;
    const auto withoutObject = worldScene(constructionGame.view(), {});
    const auto withObject =
        worldSceneWithSystems(constructionGame.view(), constructionOptions);
    require(withObject.items.size() > withoutObject.items.size(),
            "placed construction object was omitted from world scene");

    auto utilityGame = hh::game::Simulation::tutorial(1902);
    const auto utilityRoom = utilityGame.view().rooms.front().id;
    require(utilityGame
                .setRoomUtility(utilityRoom, hh::game::UtilityKind::Power, false)
                .ok,
            "utility render fixture could not disconnect power");
    const auto utilitySnapshot = utilityGame.view();
    const auto buildingSystems = utilityGame.buildingSystemsSnapshot();
    WorldViewOptions naturalOptions;
    naturalOptions.buildingSystems = &buildingSystems;
    const auto natural = worldSceneWithSystems(utilitySnapshot, naturalOptions);
    auto utilityOptions = naturalOptions;
    utilityOptions.overlay = Overlay::Utilities;
    const auto utilities =
        worldSceneWithSystems(utilitySnapshot, utilityOptions);
    require(utilities.items.size() ==
                natural.items.size() + utilitySnapshot.rooms.size(),
            "utility overlay did not emit one diagnostic layer per room");
    const auto &disconnectedDiagnostic = utilities.items[natural.items.size()];
    require(disconnectedDiagnostic.color.r > disconnectedDiagnostic.color.g,
            "utility overlay did not mark disconnected room invalid");
    require(utilityGame.view().rooms.front().id == utilityRoom,
            "utility rendering mutated authoritative state");

    std::cout << "World geometry, construction snapshots, overlays, preview "
                 "bounds and read-only rendering passed\n";
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
