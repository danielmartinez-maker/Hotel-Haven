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

std::uint32_t assetNumber(std::string_view id) {
  if (!id.starts_with("HH_A") || id.size() <= 4)
    throw std::runtime_error("unexpected world asset id");
  return static_cast<std::uint32_t>(std::stoul(std::string(id.substr(4))));
}
bool containsHandle(const RenderScene &scene, std::uint32_t handle) {
  return std::any_of(scene.meshes.begin(), scene.meshes.end(),
                     [handle](const MeshRenderItem &item) {
                       return item.asset == AssetHandle{handle};
                     });
}
bool sameBoxGeometry(const RenderScene &a, const RenderScene &b) {
  if (a.items.size() != b.items.size())
    return false;
  for (std::size_t index = 0; index < a.items.size(); ++index) {
    const auto &left = a.items[index];
    const auto &right = b.items[index];
    if (left.center.x != right.center.x || left.center.y != right.center.y ||
        left.center.z != right.center.z || left.size.x != right.size.x ||
        left.size.y != right.size.y || left.size.z != right.size.z)
      return false;
  }
  return true;
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
  for (std::size_t i = 0; i < baseline.meshes.size(); ++i) {
    const auto &a = baseline.meshes[i].tint;
    const auto &b = candidate.meshes[i].tint;
    if (a.r != b.r || a.g != b.g || a.b != b.b || a.a != b.a)
      return true;
  }
  return false;
}
} // namespace
int main() {
  try {
    std::vector<std::string> requestedIds;
    const WorldAssetSet resolved = resolveWorldAssets(
        [&](std::string_view id) {
          requestedIds.emplace_back(id);
          return visual(assetNumber(id));
        });
    require(requiredWorldAssetIds().size() == 59u,
            "world dependency contract should contain 59 assets");
    require(requestedIds.size() == requiredWorldAssetIds().size(),
            "startup world asset resolver did not resolve the complete dependency set");
    auto uniqueIds = requestedIds;
    std::sort(uniqueIds.begin(), uniqueIds.end());
    require(std::adjacent_find(uniqueIds.begin(), uniqueIds.end()) ==
                uniqueIds.end(),
            "world dependency contract contains duplicate asset ids");
    require(resolved.straightStair->handle == AssetHandle{30},
            "straight stair startup binding mismatch");
    require(resolved.standardGuestDoor->handle == AssetHandle{12},
            "guest door startup binding mismatch");
    require(resolved.lobbyEntranceDoor->handle == AssetHandle{57},
            "lobby entrance startup binding mismatch");
    require(resolved.guestBed->handle == AssetHandle{113},
            "guest bed startup binding mismatch");
    require(resolved.nightstand->handle == AssetHandle{121},
            "nightstand startup binding mismatch");
    require(resolved.bedsideLamp->handle == AssetHandle{125},
            "bedside lamp startup binding mismatch");
    require(resolved.guestDesk->handle == AssetHandle{126},
            "guest desk startup binding mismatch");
    require(resolved.deskChair->handle == AssetHandle{129},
            "desk chair startup binding mismatch");
    require(resolved.guestArmchair->handle == AssetHandle{131},
            "guest armchair startup binding mismatch");
    require(resolved.luggageBench->handle == AssetHandle{140},
            "luggage bench startup binding mismatch");
    require(resolved.wardrobe->handle == AssetHandle{141},
            "wardrobe startup binding mismatch");
    require(resolved.wallTelevision->handle == AssetHandle{153},
            "wall television startup binding mismatch");
    require(resolved.bathroomVanity->handle == AssetHandle{166},
            "bathroom vanity startup binding mismatch");
    require(resolved.bathroomToilet->handle == AssetHandle{169},
            "bathroom toilet startup binding mismatch");
    require(resolved.showerGlass->handle == AssetHandle{171},
            "shower startup binding mismatch");
    require(resolved.receptionDesk->handle == AssetHandle{186},
            "reception desk startup binding mismatch");
    require(resolved.lobbySofa->handle == AssetHandle{194},
            "lobby sofa startup binding mismatch");
    require(resolved.lobbyArmchair->handle == AssetHandle{197},
            "lobby armchair startup binding mismatch");
    require(resolved.lobbyCoffeeTable->handle == AssetHandle{200},
            "lobby coffee table startup binding mismatch");
    require(resolved.cleaningSupplyCabinet->handle == AssetHandle{302},
            "cleaning cabinet startup binding mismatch");
    require(resolved.staffLockerBank->handle == AssetHandle{338},
            "staff locker startup binding mismatch");
    require(resolved.staffBench->handle == AssetHandle{339},
            "staff bench startup binding mismatch");
    require(resolved.pottedPlant->handle == AssetHandle{396},
            "potted plant startup binding mismatch");
    require(resolved.guestCharacters.front()->handle == AssetHandle{451} &&
                resolved.guestCharacters.back()->handle == AssetHandle{480},
            "guest character variants were not fully resolved");
    require(resolved.receptionistCharacters.front()->handle == AssetHandle{481},
            "receptionist variants were not resolved");
    require(resolved.housekeeperCharacters.front()->handle == AssetHandle{487},
            "housekeeper variants were not resolved");
    require(resolved.maintenanceCharacters.front()->handle == AssetHandle{495},
            "maintenance variants were not resolved");

    auto game = hh::game::Simulation::tutorial(19);
    const auto before = game.save();
    const auto snapshot = game.view();
    require(!snapshot.rooms.empty(), "starter must contain furnished rooms");

    const WorldAssetSet assets = resolved;

    WorldViewOptions options;
    options.selected = snapshot.rooms.front().id;
    const auto scene = worldScene(snapshot, options, &assets);
    require(!scene.meshes.empty(), "world omitted asset-backed furnishing geometry");
    require(containsHandle(scene, 113), "guest bed did not use its asset handle");
    require(containsHandle(scene, 121), "nightstand did not use its asset handle");
    require(containsHandle(scene, 125), "bedside lamp did not use its asset handle");
    require(containsHandle(scene, 126), "guest desk did not use its asset handle");
    require(containsHandle(scene, 129), "desk chair did not use its asset handle");
    require(containsHandle(scene, 166), "bathroom vanity did not use its asset handle");
    require(containsHandle(scene, 169), "bathroom toilet did not use its asset handle");
    require(containsHandle(scene, 171), "shower glass did not use its asset handle");
    require(containsHandle(scene, 186), "front desk did not use its asset handle");
    require(containsHandle(scene, 396), "potted plants did not use their asset handle");
    const bool hasDoor = std::any_of(
        snapshot.tiles.begin(), snapshot.tiles.end(),
        [](const auto &tile) { return tile.kind == hh::game::TileKind::Door; });
    const bool hasSupplyCloset = std::any_of(
        snapshot.tiles.begin(), snapshot.tiles.end(),
        [](const auto &tile) {
          return tile.kind == hh::game::TileKind::SupplyCloset;
        });
    if (hasDoor)
      require(containsHandle(scene, 12), "doors did not use the guest door mesh");
    if (hasSupplyCloset)
      require(containsHandle(scene, 302),
              "supply closets did not use the cleaning cabinet mesh");
    require(scene.focusTarget.has_value(), "selected room has no cutaway focus");

    hh::game::SimulationView placementSnapshot;
    placementSnapshot.width = 12;
    placementSnapshot.height = 10;
    placementSnapshot.floors = 1;
    placementSnapshot.tiles = {
        {{0, 0, 0}, hh::game::TileKind::Lobby},
        {{0, 2, 0}, hh::game::TileKind::Lobby},
        {{0, 4, 0}, hh::game::TileKind::Lobby},
        {{0, 6, 0}, hh::game::TileKind::StaffRoom},
        {{0, 8, 0}, hh::game::TileKind::Entrance},
    };
    hh::game::RoomView placementRoom;
    placementRoom.id = 700001;
    placementRoom.name = "Asset placement room";
    placementRoom.door = {0, 1, 1};
    placementRoom.status = hh::game::RoomStatus::VacantReady;
    placementRoom.floor = 0;
    placementRoom.x = 1;
    placementRoom.y = 2;
    placementRoom.width = 7;
    placementRoom.height = 7;
    placementRoom.beds = 1;
    placementRoom.baths = 1;
    placementRoom.cleanliness = 100;
    placementRoom.condition = 100;
    placementSnapshot.rooms.push_back(placementRoom);

    const auto placementScene =
        worldScene(placementSnapshot, WorldViewOptions{}, &assets);
    for (const auto handle : std::array<std::uint32_t, 10>{
             57, 131, 140, 141, 153, 194, 197, 200, 338, 339}) {
      require(containsHandle(placementScene, handle),
              "world presentation omitted an integrated room/public-area asset");
    }

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

    hh::game::PersonView receptionist;
    receptionist.id = 900004;
    receptionist.name = "Overlay receptionist";
    receptionist.kind = hh::game::PersonKind::Receptionist;
    receptionist.state = hh::game::PersonState::Working;
    receptionist.position = {0, 5, 8};
    receptionist.onShift = true;
    diagnosticSnapshot.people.push_back(receptionist);

    hh::game::PersonView maintenance;
    maintenance.id = 900005;
    maintenance.name = "Overlay maintenance";
    maintenance.kind = hh::game::PersonKind::Maintenance;
    maintenance.state = hh::game::PersonState::Working;
    maintenance.position = {0, 7, 8};
    maintenance.onShift = true;
    diagnosticSnapshot.people.push_back(maintenance);

    hh::game::TaskView task;
    task.id = 900003;
    task.kind = hh::game::TaskKind::Turnover;
    task.status = hh::game::TaskStatus::Blocked;
    task.targetId = diagnosticSnapshot.rooms.front().id;
    task.target = diagnosticSnapshot.rooms.front().door;
    diagnosticSnapshot.tasks.push_back(task);

    options.overlay = Overlay::Natural;
    const auto diagnosticBaseline = worldScene(diagnosticSnapshot, options, &assets);
    require(containsHandle(diagnosticBaseline, 452),
            "guest character variant was not rendered");
    require(containsHandle(diagnosticBaseline, 487),
            "housekeeper character variant was not rendered");
    require(containsHandle(diagnosticBaseline, 481),
            "receptionist character variant was not rendered");
    require(containsHandle(diagnosticBaseline, 496),
            "maintenance character variant was not rendered");
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

    hh::game::SimulationView motionSnapshot;
    motionSnapshot.width = 10;
    motionSnapshot.height = 10;
    motionSnapshot.floors = 1;
    motionSnapshot.elapsedSeconds = 1;
    hh::game::PersonView traveler;
    traveler.id = 7;
    traveler.name = "Motion traveler";
    traveler.kind = hh::game::PersonKind::Guest;
    traveler.state = hh::game::PersonState::Traveling;
    traveler.position = {0, 3, 3};
    motionSnapshot.people.push_back(traveler);

    WorldViewOptions motionOptions;
    const auto animatedMotion = worldScene(motionSnapshot, motionOptions, nullptr);
    motionOptions.reducedMotion = true;
    const auto reducedMotion = worldScene(motionSnapshot, motionOptions, nullptr);
    require(!sameBoxGeometry(animatedMotion, reducedMotion),
            "reduced motion did not suppress procedural walking gait");

    auto laterMotionSnapshot = motionSnapshot;
    laterMotionSnapshot.elapsedSeconds = 47;
    const auto reducedMotionLater =
        worldScene(laterMotionSnapshot, motionOptions, nullptr);
    require(sameBoxGeometry(reducedMotion, reducedMotionLater),
            "reduced-motion world geometry still depends on presentation time");

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
