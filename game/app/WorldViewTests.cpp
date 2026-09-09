#include "WorldView.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace hh::client;
using namespace hh::renderer;
static void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}
namespace {
WorldAssetVisual visual(std::uint32_t handle) {
  return WorldAssetVisual{
      AssetHandle{handle}, Aabb{{-0.5f, 0.0f, -0.5f}, {0.5f, 1.0f, 0.5f}}, false};
}
bool containsHandle(const RenderScene &scene, std::uint32_t handle) {
  return std::any_of(scene.meshes.begin(), scene.meshes.end(),
                     [handle](const MeshRenderItem &item) {
                       return item.asset == AssetHandle{handle};
                     });
}
} // namespace
int main() {
  try {
    auto game = hh::game::Simulation::tutorial(19);
    const auto before = game.save();
    const auto snapshot = game.view();
    require(!snapshot.rooms.empty(), "starter must contain furnished rooms");

    WorldAssetSet assets;
    assets.straightStair = visual(30);
    assets.guestBed = visual(113);
    assets.nightstand = visual(121);
    assets.guestDesk = visual(126);
    assets.bathroomVanity = visual(166);
    assets.bathroomToilet = visual(169);
    assets.showerGlass = visual(171);
    assets.receptionDesk = visual(186);
    assets.pottedPlant = visual(396);

    WorldViewOptions options;
    options.selected = snapshot.rooms.front().id;
    const auto scene = worldScene(snapshot, options, &assets);
    require(!scene.meshes.empty(), "world omitted asset-backed furnishing geometry");
    require(containsHandle(scene, 113), "guest bed did not use its asset handle");
    require(containsHandle(scene, 121), "nightstand did not use its asset handle");
    require(containsHandle(scene, 126), "guest desk did not use its asset handle");
    require(containsHandle(scene, 166), "bathroom vanity did not use its asset handle");
    require(containsHandle(scene, 169), "bathroom toilet did not use its asset handle");
    require(containsHandle(scene, 171), "shower glass did not use its asset handle");
    require(containsHandle(scene, 186), "front desk did not use its asset handle");
    require(containsHandle(scene, 396), "potted plants did not use their asset handle");
    require(scene.focusTarget.has_value(), "selected room has no cutaway focus");

    for (const auto &item : scene.items) {
      require(std::isfinite(item.center.x) && std::isfinite(item.center.y) &&
                  std::isfinite(item.center.z),
              "non-finite box geometry");
      require(item.size.x > 0 && item.size.y > 0 && item.size.z > 0,
              "non-positive box dimension");
      require(item.bounds.min.x <= item.center.x &&
                  item.bounds.max.x >= item.center.x,
              "invalid box bounds");
    }
    for (const auto &item : scene.meshes) {
      require(std::isfinite(item.transform.translation.x) &&
                  std::isfinite(item.transform.translation.y) &&
                  std::isfinite(item.transform.translation.z),
              "non-finite mesh translation");
      require(item.transform.scale.x > 0 && item.transform.scale.y > 0 &&
                  item.transform.scale.z > 0,
              "non-positive mesh scale");
      require(item.bounds.max.x > item.bounds.min.x &&
                  item.bounds.max.y > item.bounds.min.y &&
                  item.bounds.max.z > item.bounds.min.z,
              "invalid mesh world bounds");
    }

    options.overlay = Overlay::Cleanliness;
    const auto heatmap = worldScene(snapshot, options, &assets);
    require(heatmap.items.size() == scene.items.size(),
            "heatmap changed procedural diagnostic geometry");
    require(heatmap.meshes.size() == scene.meshes.size(),
            "heatmap changed asset-backed physical geometry");
    bool changed = false;
    for (std::size_t i = 0; i < scene.items.size(); ++i)
      changed |= scene.items[i].color.r != heatmap.items[i].color.r;
    require(changed, "cleanliness heatmap has no diagnostic colors");

    options.showPreview = true;
    options.previewSize = 6;
    options.previewValid = false;
    options.hoverX = snapshot.width - 1;
    options.hoverY = snapshot.height - 1;
    const auto ghost = worldScene(snapshot, options, &assets).items.back();
    require(ghost.size.x == 1 && ghost.size.z == 1,
            "out-of-map preview was not clipped");
    require(ghost.color.r > ghost.color.g, "invalid preview must be red");
    require(game.save() == before,
            "world rendering mutated authoritative state");
    std::cout << "World assets, overlays, preview bounds and read-only rendering passed\n";
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
