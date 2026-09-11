#include "WorldView.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
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
bool hasDifferentDiagnosticColor(const RenderScene &baseline,
                                 const RenderScene &candidate) {
  if (candidate.items.size() != baseline.items.size() ||
      candidate.meshes.size() != baseline.meshes.size())
    return false;
  for (std::size_t i = 0; i < baseline.items.size(); ++i) {
    const auto &a = baseline.items[i].color;
    const auto &b = candidate.items[i].color;
    if (a.r != b.r || a.g != b.g || a.b != b.b || a.a != b.a)
      return true;
  }
  return false;
}
} // namespace
int main() {
  try {
    const std::map<std::string, std::uint32_t> expectedIds{
        {"HH_A030", 30},  {"HH_A113", 113}, {"HH_A121", 121},
        {"HH_A126", 126}, {"HH_A166", 166}, {"HH_A169", 169},
        {"HH_A171", 171}, {"HH_A186", 186}, {"HH_A396", 396},
    };
    std::vector<std::string> requestedIds;
    const WorldAssetSet resolved = resolveWorldAssets(
        [&](std::string_view id) {
          requestedIds.emplace_back(id);
          const auto found = expectedIds.find(std::string(id));
          if (found == expectedIds.end())
            throw std::runtime_error("unexpected world asset id");
          return visual(found->second);
        });
    require(requestedIds.size() == expectedIds.size(),
            "startup world asset resolver did not resolve exactly nine assets");
    require(resolved.straightStair->handle == AssetHandle{30},
            "straight stair startup binding mismatch");
    require(resolved.guestBed->handle == AssetHandle{113},
            "guest bed startup binding mismatch");
    require(resolved.nightstand->handle == AssetHandle{121},
            "nightstand startup binding mismatch");
    require(resolved.guestDesk->handle == AssetHandle{126},
            "guest desk startup binding mismatch");
    require(resolved.bathroomVanity->handle == AssetHandle{166},
            "bathroom vanity startup binding mismatch");
    require(resolved.bathroomToilet->handle == AssetHandle{169},
            "bathroom toilet startup binding mismatch");
    require(resolved.showerGlass->handle == AssetHandle{171},
            "shower startup binding mismatch");
    require(resolved.receptionDesk->handle == AssetHandle{186},
            "reception desk startup binding mismatch");
    require(resolved.pottedPlant->handle == AssetHandle{396},
            "potted plant startup binding mismatch");

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
    require(hasDifferentDiagnosticColor(scene, heatmap),
            "cleanliness heatmap has no diagnostic colors");

    // The tutorial starts at 14:00 before its first simulation step, so its
    // staff remain OffDuty and no guest/task samples exist yet. Exercise the
    // renderer seam with explicit snapshot data instead of assuming unavailable
    // authority exists at campaign creation.
    auto diagnosticSnapshot = snapshot;
    hh::game::PersonView guest;
    guest.id = 900001;
    guest.name = "Overlay guest";
    guest.kind = hh::game::PersonKind::Guest;
    guest.state = hh::game::PersonState::Waiting;
    guest.position = {0, 1, 8};
    guest.satisfaction = 20.0;
    guest.queueWaitSeconds = 300;
    diagnosticSnapshot.people.push_back(guest);

    hh::game::PersonView employee;
    employee.id = 900002;
    employee.name = "Overlay employee";
    employee.kind = hh::game::PersonKind::Housekeeper;
    employee.state = hh::game::PersonState::Working;
    employee.position = {0, 3, 8};
    employee.onShift = true;
    diagnosticSnapshot.people.push_back(employee);

    hh::game::TaskView task;
    task.id = 900003;
    task.kind = hh::game::TaskKind::Turnover;
    task.status = hh::game::TaskStatus::Blocked;
    task.targetId = diagnosticSnapshot.rooms.front().id;
    task.target = diagnosticSnapshot.rooms.front().door;
    diagnosticSnapshot.tasks.push_back(task);

    options.overlay = Overlay::Natural;
    const auto diagnosticBaseline = worldScene(diagnosticSnapshot, options, &assets);
    for (const auto mode : std::array{
             Overlay::GuestSatisfaction, Overlay::StaffUtilization,
             Overlay::QueueWait, Overlay::OpenTaskDensity}) {
      options.overlay = mode;
      const auto diagnostic = worldScene(diagnosticSnapshot, options, &assets);
      require(diagnostic.items.size() == diagnosticBaseline.items.size(),
              "management overlay changed procedural geometry");
      require(diagnostic.meshes.size() == diagnosticBaseline.meshes.size(),
              "management overlay changed asset-backed geometry");
      require(hasDifferentDiagnosticColor(diagnosticBaseline, diagnostic),
              "available management overlay has no world diagnostic colors");
    }

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
    std::cout << "World asset bindings, geometry, overlays and read-only rendering passed\n";
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
