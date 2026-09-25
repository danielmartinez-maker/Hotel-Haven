#include "WorldView.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <map>
#include <tuple>
namespace hh::client {
using namespace hh::renderer;
using namespace hh::game;
namespace {
const Color wood{.52f, .32f, .19f, 1}, cream{.93f, .88f, .74f, 1},
    teal{.16f, .47f, .43f, 1}, gold{.83f, .62f, .30f, 1};

void box(RenderScene &s, int floor, float x, float z, float y, float w, float d,
         float h, Color c, RenderCategory category = RenderCategory::Object) {
  BoxRenderItem item;
  item.floorId = floor;
  item.center = {x, static_cast<float>(floor) * 3.2f + y, z};
  item.size = {w, h, d};
  item.color = c;
  item.category = category;
  const auto half = item.size * .5f;
  item.bounds = {item.center - half, item.center + half};
  s.items.push_back(item);
}

Vec3 rotateY(Vec3 point, float radians) {
  const float cosine = std::cos(radians), sine = std::sin(radians);
  return {point.x * cosine + point.z * sine, point.y,
          -point.x * sine + point.z * cosine};
}

bool fittedMesh(RenderScene &scene, int floor, float x, float z, float y,
                float width, float depth, float height,
                const std::optional<WorldAssetVisual> &visual,
                Color tint = {1, 1, 1, 1},
                RenderCategory category = RenderCategory::Object,
                float yawRadians = 0) {
  if (!visual)
    return false;
  const Vec3 localSize = visual->localBounds.max - visual->localBounds.min;
  constexpr float epsilon = 1e-5f;
  if (localSize.x <= epsilon || localSize.y <= epsilon ||
      localSize.z <= epsilon || width <= 0 || depth <= 0 || height <= 0)
    return false;

  MeshRenderItem item;
  item.asset = visual->handle;
  item.floorId = floor;
  item.category = category;
  item.tint = tint;
  item.translucent = visual->translucent || tint.a < .999f;
  item.transform.scale = {width / localSize.x, height / localSize.y,
                          depth / localSize.z};
  item.transform.yawRadians = yawRadians;

  const Vec3 localCenter = (visual->localBounds.min + visual->localBounds.max) * .5f;
  const Vec3 scaledCenter{localCenter.x * item.transform.scale.x,
                          localCenter.y * item.transform.scale.y,
                          localCenter.z * item.transform.scale.z};
  const Vec3 rotatedCenter = rotateY(scaledCenter, yawRadians);
  const Vec3 desiredCenter{x, static_cast<float>(floor) * 3.2f + y, z};
  item.transform.translation = desiredCenter - rotatedCenter;

  const std::array<Vec3, 8> corners{{
      {visual->localBounds.min.x, visual->localBounds.min.y,
       visual->localBounds.min.z},
      {visual->localBounds.min.x, visual->localBounds.min.y,
       visual->localBounds.max.z},
      {visual->localBounds.min.x, visual->localBounds.max.y,
       visual->localBounds.min.z},
      {visual->localBounds.min.x, visual->localBounds.max.y,
       visual->localBounds.max.z},
      {visual->localBounds.max.x, visual->localBounds.min.y,
       visual->localBounds.min.z},
      {visual->localBounds.max.x, visual->localBounds.min.y,
       visual->localBounds.max.z},
      {visual->localBounds.max.x, visual->localBounds.max.y,
       visual->localBounds.min.z},
      {visual->localBounds.max.x, visual->localBounds.max.y,
       visual->localBounds.max.z},
  }};
  Vec3 minimum{1e30f, 1e30f, 1e30f};
  Vec3 maximum{-1e30f, -1e30f, -1e30f};
  for (const Vec3 corner : corners) {
    const Vec3 scaled{corner.x * item.transform.scale.x,
                      corner.y * item.transform.scale.y,
                      corner.z * item.transform.scale.z};
    const Vec3 world = rotateY(scaled, yawRadians) + item.transform.translation;
    minimum.x = std::min(minimum.x, world.x);
    minimum.y = std::min(minimum.y, world.y);
    minimum.z = std::min(minimum.z, world.z);
    maximum.x = std::max(maximum.x, world.x);
    maximum.y = std::max(maximum.y, world.y);
    maximum.z = std::max(maximum.z, world.z);
  }
  item.bounds = {minimum, maximum};
  scene.meshes.push_back(item);
  return true;
}

std::string milestoneAssetId(std::uint32_t number) {
  char buffer[16]{};
  std::snprintf(buffer, sizeof(buffer), "HH_A%03u", number);
  return buffer;
}

bool fittedCatalogMesh(RenderScene &scene, int floor, float x, float z, float y,
                       float width, float depth, float height,
                       const WorldAssetSet *assets, std::string_view assetId,
                       Color tint = {1, 1, 1, 1},
                       RenderCategory category = RenderCategory::Object,
                       float yawRadians = 0) {
  if (!assets)
    return false;
  const WorldAssetVisual *visual = findWorldAsset(*assets, assetId);
  if (!visual)
    return false;
  const std::optional<WorldAssetVisual> wrapped{*visual};
  return fittedMesh(scene, floor, x, z, y, width, depth, height, wrapped,
                    tint, category, yawRadians);
}

void plant(RenderScene &s, int f, float x, float z,
           const WorldAssetSet *assets) {
  if (assets && fittedMesh(s, f, x, z, .58f, .75f, .70f, 1.16f,
                           assets->pottedPlant))
    return;
  box(s, f, x, z, .20f, .34f, .34f, .4f, {.70f, .42f, .29f, 1});
  box(s, f, x, z, .68f, .70f, .62f, .60f, {.23f, .42f, .25f, 1});
  box(s, f, x + .1f, z, .96f, .45f, .48f, .4f, {.31f, .51f, .29f, 1});
}

void lobbyDecoration(RenderScene &s, const TileView &tile, float x, float z,
                     const WorldAssetSet *assets) {
  if (!assets)
    return;

  int motif = (tile.position.x * 31 + tile.position.y * 17 +
               tile.position.floor * 13) %
              10;
  if (motif < 0)
    motif += 10;

  bool rendered = false;
  if (motif == 0) {
    rendered = fittedCatalogMesh(s, tile.position.floor, x, z, .40f,
                                 .88f, .55f, .80f, assets, "HH_A609");
    if (!rendered)
      (void)fittedMesh(s, tile.position.floor, x, z, .40f, .88f, .55f, .80f,
                       assets->lobbySofa);
  } else if (motif == 1) {
    rendered = fittedCatalogMesh(s, tile.position.floor, x, z, .68f,
                                 .34f, .34f, 1.36f, assets, "HH_A619");
    if (!rendered)
      (void)fittedMesh(s, tile.position.floor, x, z, .68f, .34f, .34f, 1.36f,
                       assets->lobbyFloorLamp);
  } else if (motif == 2) {
    rendered = fittedCatalogMesh(s, tile.position.floor, x, z, .43f,
                                 .68f, .68f, .86f, assets, "HH_A611");
    if (!rendered)
      (void)fittedMesh(s, tile.position.floor, x, z, .43f, .68f, .68f, .86f,
                       assets->lobbyArmchair);
  } else if (motif == 3) {
    rendered = fittedCatalogMesh(s, tile.position.floor, x, z, .52f,
                                 .72f, .34f, 1.04f, assets, "HH_A623");
    if (!rendered)
      (void)fittedMesh(s, tile.position.floor, x, z, .58f, .58f, .58f, 1.16f,
                       assets->lobbyPlanter);
  } else if (motif == 4) {
    rendered = fittedCatalogMesh(s, tile.position.floor, x, z, .19f,
                                 .70f, .55f, .38f, assets, "HH_A615");
    if (!rendered)
      (void)fittedMesh(s, tile.position.floor, x, z, .19f, .70f, .55f, .38f,
                       assets->lobbyCoffeeTable);
  } else if (motif == 5) {
    rendered = fittedCatalogMesh(s, tile.position.floor, x, z, .60f,
                                 .58f, .68f, 1.20f, assets, "HH_A608");
    if (!rendered)
      (void)fittedMesh(s, tile.position.floor, x, z, .60f, .58f, .68f, 1.20f,
                       assets->luggageCart);
  } else if (motif == 6) {
    rendered = fittedCatalogMesh(s, tile.position.floor, x, z, .40f,
                                 .96f, .58f, .80f, assets, "HH_A610");
    if (!rendered)
      (void)fittedMesh(s, tile.position.floor, x, z, .40f, .96f, .58f, .80f,
                       assets->lobbySofa);
  } else if (motif == 7) {
    rendered = fittedCatalogMesh(s, tile.position.floor, x, z, .24f,
                                 .62f, .58f, .48f, assets, "HH_A614");
    if (!rendered)
      (void)fittedMesh(s, tile.position.floor, x, z, .34f, .58f, .58f, .68f,
                       assets->lobbyArmchair);
  } else if (motif == 8) {
    rendered = fittedCatalogMesh(s, tile.position.floor, x, z, .58f,
                                 .46f, .38f, 1.16f, assets, "HH_A643");
    if (!rendered)
      (void)fittedMesh(s, tile.position.floor, x, z, .58f, .46f, .38f, 1.16f,
                       assets->pottedPlant);
  } else {
    rendered = fittedCatalogMesh(s, tile.position.floor, x, z, .64f,
                                 .54f, .42f, 1.28f, assets, "HH_A644");
    if (!rendered)
      (void)fittedMesh(s, tile.position.floor, x, z, .58f, .54f, .42f, 1.16f,
                       assets->pottedPlant);
  }
}

void staffRoomFurniture(RenderScene &s, int floor, float x, float z,
                        const WorldAssetSet *assets) {
  if (!assets)
    return;
  (void)fittedMesh(s, floor, x, z + .24f, .75f, .70f, .34f, 1.50f,
                   assets->staffLockerBank);
  (void)fittedMesh(s, floor, x, z - .27f, .24f, .72f, .34f, .48f,
                   assets->staffBench);
}

struct BedPresentation {
  const std::optional<WorldAssetVisual> *visual{};
  std::string_view milestoneAssetId;
  float width{1.62f};
};

BedPresentation roomBedPresentation(
    const WorldAssetSet *assets, const RoomView &room, int bedIndex) {
  const bool twinLayout = room.beds > 1 && room.width >= 8;
  if (twinLayout) {
    return {
        assets ? (bedIndex == 0 ? &assets->twinBedLeft : &assets->twinBedRight)
               : nullptr,
        "HH_A554",
        .95f,
    };
  }

  if (room.width <= 4)
    return {assets ? &assets->singleBed : nullptr, "HH_A554", 1.00f};
  if (room.width <= 5)
    return {assets ? &assets->doubleBed : nullptr, "HH_A551", 1.40f};
  if (room.width >= 8)
    return {assets ? &assets->kingBed : nullptr, "HH_A553", 1.90f};
  return {assets ? &assets->queenBed : nullptr, "HH_A552", 1.62f};
}

void bed(RenderScene &s, int f, float x, float z,
         const BedPresentation &presentation, const WorldAssetSet *assets) {
  bool assetBed = false;
  if (assets && !presentation.milestoneAssetId.empty()) {
    assetBed = fittedCatalogMesh(
        s, f, x, z, .58f, presentation.width, 2.10f, 1.16f,
        assets, presentation.milestoneAssetId);
  }
  if (!assetBed && presentation.visual) {
    assetBed = fittedMesh(s, f, x, z, .58f, presentation.width, 2.10f, 1.16f,
                          *presentation.visual);
  }
  if (!assetBed) {
    const float frameWidth = presentation.width * .94f;
    const float linenWidth = presentation.width * .90f;
    box(s, f, x, z, .28f, frameWidth, 2.1f, .38f, wood);
    box(s, f, x, z, .52f, linenWidth, 2.0f, .23f, cream);
    box(s, f, x, z + .3f, .65f, linenWidth, 1.35f, .12f, teal);
    box(s, f, x, z - .72f, .72f, presentation.width * .77f, .42f, .17f,
        {.99f, .97f, .88f, 1});
    box(s, f, x, z - 1.05f, .67f, presentation.width, .13f, 1.0f, wood);
  }

  const float nightstandX = x + presentation.width * .5f + .40f;
  bool assetNightstand =
      fittedCatalogMesh(s, f, nightstandX, z - .65f, .40f,
                        .50f, .52f, .80f, assets, "HH_A557");
  if (!assetNightstand && assets)
    assetNightstand = fittedMesh(s, f, nightstandX, z - .65f, .40f,
                                 .50f, .52f, .80f, assets->nightstand);
  if (!assetNightstand)
    box(s, f, nightstandX, z - .65f, .40f, .50f, .52f, .8f, wood);
  bool assetLamp =
      fittedCatalogMesh(s, f, nightstandX, z - .65f, .98f,
                        .32f, .32f, .28f, assets, "HH_A559");
  if (!assetLamp && assets)
    assetLamp = fittedMesh(s, f, nightstandX, z - .65f, .98f,
                           .32f, .32f, .28f, assets->bedsideLamp);
  if (!assetLamp)
    box(s, f, nightstandX, z - .65f, .98f, .32f, .32f, .28f, gold);

  if (assets && presentation.width >= 1.40f) {
    const float secondNightstandX = x - presentation.width * .5f - .40f;
    if (fittedCatalogMesh(s, f, secondNightstandX, z - .65f, .40f,
                          .50f, .52f, .80f, assets, "HH_A558")) {
      (void)fittedCatalogMesh(s, f, secondNightstandX, z - .65f, .98f,
                              .32f, .32f, .28f, assets, "HH_A559");
    }
  }
}

Color statusColor(RoomStatus st) {
  switch (st) {
  case RoomStatus::VacantReady:
    return {.30f, .68f, .49f, 1};
  case RoomStatus::Occupied:
    return {.27f, .55f, .78f, 1};
  case RoomStatus::Reserved:
    return {.61f, .51f, .76f, 1};
  case RoomStatus::Cleaning:
    return {.80f, .68f, .30f, 1};
  case RoomStatus::VacantDirty:
    return {.84f, .43f, .24f, 1};
  default:
    return {.72f, .24f, .27f, 1};
  }
}

Color diagnosticGoodness(float value) {
  const float v = std::clamp(value, 0.0f, 1.0f);
  return {.85f - .55f * v, .28f + .40f * v, .22f + .22f * v, 1};
}

const std::optional<WorldAssetVisual> &characterVisual(
    const WorldAssetSet &assets, const PersonView &person) {
  switch (person.kind) {
  case PersonKind::Guest:
    return assets.guestCharacters[
        static_cast<std::size_t>(person.id) % assets.guestCharacters.size()];
  case PersonKind::Receptionist:
    return assets.receptionistCharacters[
        static_cast<std::size_t>(person.id) %
        assets.receptionistCharacters.size()];
  case PersonKind::Housekeeper:
    return assets.housekeeperCharacters[
        static_cast<std::size_t>(person.id) %
        assets.housekeeperCharacters.size()];
  case PersonKind::Maintenance:
    return assets.maintenanceCharacters[
        static_cast<std::size_t>(person.id) %
        assets.maintenanceCharacters.size()];
  }
  return assets.guestCharacters.front();
}
} // namespace

RenderScene worldScene(const SimulationView &snapshot, const WorldViewOptions &c,
                       const WorldAssetSet *assets) {
  RenderScene s;
  s.activeFloor = c.floor;

  std::map<EntityId, int> openTaskCount;
  if (c.overlay == Overlay::OpenTaskDensity) {
    for (const auto &task : snapshot.tasks) {
      if (task.status != TaskStatus::Completed && task.targetId != 0)
        ++openTaskCount[task.targetId];
    }
  }

  // The garden is presentation scenery; construction remains inside map bounds.
  box(s, 0, static_cast<float>(snapshot.width) * .5f,
      static_cast<float>(snapshot.height) * .5f, -.20f,
      static_cast<float>(snapshot.width) + 12,
      static_cast<float>(snapshot.height) + 12, .25f, {.46f, .55f, .40f, 1},
      RenderCategory::Floor);
  for (int x = -3; x < snapshot.width + 3; x += 4)
    plant(s, 0, static_cast<float>(x), -2.5f, assets);

  for (const auto &t : snapshot.tiles) {
    if (t.kind == TileKind::Empty)
      continue;
    const int f = t.position.floor;
    const float x = static_cast<float>(t.position.x) + .5f,
                z = static_cast<float>(t.position.y) + .5f;
    const float shade = ((t.position.x + t.position.y) % 2 == 0) ? 1.f : .97f;
    box(s, f, x, z, -.015f, .99f, .99f, .14f,
        {.78f * shade, .76f * shade, .66f * shade, 1}, RenderCategory::Floor);
    if (t.kind == TileKind::Lobby) {
      box(s, f, x, z, .075f, .88f, .88f, .04f,
          {.18f * shade, .45f * shade, .43f * shade, 1}, RenderCategory::Floor);
      box(s, f, x, z, .10f, .16f, .16f, .05f, gold, RenderCategory::Floor);
      lobbyDecoration(s, t, x, z, assets);
    } else if (t.kind == TileKind::Wall) {
      box(s, f, x, z, 1.3f, .97f, .97f, 2.6f, {.87f, .82f, .70f, 1},
          RenderCategory::Wall);
      box(s, f, x, z, .12f, 1.0f, 1.0f, .15f, wood);
    } else if (t.kind == TileKind::Door) {
      const bool assetDoor = assets &&
          fittedMesh(s, f, x, z, 1.f, .98f, .22f, 2.f,
                     assets->standardGuestDoor);
      if (!assetDoor) {
        box(s, f, x - .43f, z, 1.f, .12f, .22f, 2.f, wood);
        box(s, f, x + .43f, z, 1.f, .12f, .22f, 2.f, wood);
        box(s, f, x, z, 2.02f, .98f, .22f, .13f, wood);
      }
    } else if (t.kind == TileKind::FrontDesk) {
      const std::uint32_t deskVariant =
          601u + static_cast<std::uint32_t>(
                     std::abs(t.position.x * 7 + t.position.y * 11) % 5);
      bool assetDesk = fittedCatalogMesh(
          s, f, x, z, .70f, 1.73f, .83f, 1.40f, assets,
          milestoneAssetId(deskVariant));
      if (!assetDesk && assets)
        assetDesk = fittedMesh(s, f, x, z, .70f, 1.73f, .83f, 1.40f,
                               assets->receptionDesk);
      if (!assetDesk) {
        box(s, f, x, z, .52f, 1.6f, .75f, 1.04f, teal);
        box(s, f, x, z, 1.08f, 1.73f, .83f, .13f, cream);
        box(s, f, x + .25f, z, 1.33f, .4f, .12f, .36f,
            {.18f, .21f, .22f, 1});
        box(s, f, x - .40f, z, 1.18f, .14f, .14f, .09f, gold);
      }
    } else if (t.kind == TileKind::SupplyCloset) {
      const bool assetCabinet = assets &&
          fittedMesh(s, f, x, z, .8f, .8f, .75f, 1.6f,
                     assets->cleaningSupplyCabinet);
      if (!assetCabinet) {
        box(s, f, x, z, .8f, .8f, .75f, 1.6f, wood);
        for (int i = 0; i < 3; ++i)
          box(s, f, x, z - .04f, .30f + static_cast<float>(i) * .48f, .70f,
              .66f, .26f, cream);
      }
    } else if (t.kind == TileKind::Entrance) {
      const bool assetEntrance = assets &&
          fittedMesh(s, f, x, z, 1.02f, .92f, .18f, 2.04f,
                     assets->lobbyEntranceDoor);
      if (!assetEntrance)
        box(s, f, x, z, .05f, .96f, .96f, .05f, teal);
      (void)fittedCatalogMesh(s, f, x, z, .05f, .92f, .72f, .10f,
                              assets, "HH_A540");
      (void)fittedCatalogMesh(s, f, x, z, 1.08f, 1.10f, .24f, 2.16f,
                              assets, "HH_A539");
    } else if (t.kind == TileKind::StaffRoom) {
      staffRoomFurniture(s, f, x, z, assets);
    } else if (t.kind == TileKind::Stairs) {
      const std::uint32_t stairVariant =
          511u + static_cast<std::uint32_t>(
                     std::abs(t.position.x + t.position.y + t.position.floor) % 3);
      bool assetStair = fittedCatalogMesh(
          s, f, x, z, .48f, .85f, 1.0f, .96f, assets,
          milestoneAssetId(stairVariant));
      if (!assetStair && stairVariant != 511u)
        assetStair = fittedCatalogMesh(
            s, f, x, z, .48f, .85f, 1.0f, .96f, assets, "HH_A511");
      if (!assetStair && assets)
        assetStair = fittedMesh(s, f, x, z, .48f, .85f, 1.0f, .96f,
                                assets->straightStair);
      if (!assetStair)
        for (int i = 0; i < 5; ++i)
          box(s, f, x, z - .4f + static_cast<float>(i) * .2f,
              .08f + static_cast<float>(i) * .12f, .85f, .2f,
              .15f + static_cast<float>(i) * .24f, cream);
    }
  }

  for (const auto &r : snapshot.rooms) {
    const int f = r.door.floor;
    const float x = static_cast<float>(r.x), z = static_cast<float>(r.y);
    const float w = static_cast<float>(r.width),
                d = static_cast<float>(r.height);
    Color rug{.67f, .52f, .35f, 1};
    if (c.overlay == Overlay::Status)
      rug = statusColor(r.status);
    if (c.overlay == Overlay::Cleanliness)
      rug = diagnosticGoodness(static_cast<float>(r.cleanliness) / 100.0f);
    if (c.overlay == Overlay::Condition)
      rug = diagnosticGoodness(static_cast<float>(r.condition) / 100.0f);
    if (c.overlay == Overlay::OpenTaskDensity) {
      const int count = openTaskCount[r.id];
      const float goodness = 1.0f - std::min(1.0f, static_cast<float>(count) / 4.0f);
      rug = diagnosticGoodness(goodness);
    }
    box(s, f, x + w * .5f, z + d * .5f, .07f, std::max(1.f, w - 2.1f),
        std::max(1.f, d - 2.1f), .05f, rug);
    bed(s, f, x + 2.f, z + 2.5f,
        roomBedPresentation(assets, r, 0), assets);
    if (r.beds > 1 && w >= 8)
      bed(s, f, x + w - 2.f, z + 2.5f,
          roomBedPresentation(assets, r, 1), assets);

    // Bathroom fixtures remain presentation-only; room simulation owns baths.
    if (r.baths > 0) {
      box(s, f, x + w - 1.7f, z + d - 1.6f, .09f, 1.7f, 1.6f, .07f,
          {.80f, .88f, .84f, 1});
      bool assetToilet = fittedCatalogMesh(
          s, f, x + w - 1.35f, z + d - 1.5f, .26f, .48f, .67f, .45f,
          assets, "HH_A575");
      if (!assetToilet && assets)
        assetToilet = fittedMesh(s, f, x + w - 1.35f, z + d - 1.5f, .26f,
                                 .48f, .67f, .45f, assets->bathroomToilet);
      if (!assetToilet)
        box(s, f, x + w - 1.35f, z + d - 1.5f, .26f, .48f, .67f, .45f,
            cream);
      box(s, f, x + w - 1.35f, z + d - 1.75f, .61f, .5f, .22f, .5f,
          cream);
      bool assetVanity = fittedCatalogMesh(
          s, f, x + w - 2.1f, z + d - 1.35f, .59f, .65f, .55f, 1.18f,
          assets, "HH_A572");
      if (!assetVanity && assets)
        assetVanity = fittedMesh(s, f, x + w - 2.1f, z + d - 1.35f, .59f,
                                 .65f, .55f, 1.18f, assets->bathroomVanity);
      if (!assetVanity) {
        box(s, f, x + w - 2.1f, z + d - 1.35f, .57f, .56f, .49f, 1.1f,
            wood);
        box(s, f, x + w - 2.1f, z + d - 1.35f, 1.15f, .65f, .55f, .09f,
            cream);
      }
      bool assetShower = fittedCatalogMesh(
          s, f, x + w - 2.65f, z + d - 1.75f, .72f, .10f, 1.4f, 1.3f,
          assets, "HH_A577", {1.f, 1.f, 1.f, .62f});
      if (!assetShower && assets)
        assetShower = fittedMesh(s, f, x + w - 2.65f, z + d - 1.75f, .72f,
                                 .10f, 1.4f, 1.3f, assets->showerGlass,
                                 {1.f, 1.f, 1.f, .62f});
      if (!assetShower)
        box(s, f, x + w - 2.65f, z + d - 1.75f, .72f, .10f, 1.4f, 1.3f,
            {.63f, .79f, .77f, .55f});
      // Small bathroom details share the authoritative room/bath footprint;
      // they add presentation fidelity without creating a second placement
      // model or interaction authority.
      (void)fittedCatalogMesh(s, f, x + w - 2.65f, z + d - 1.75f, .08f,
                              .82f, 1.12f, .12f, assets, "HH_A578");
      (void)fittedCatalogMesh(s, f, x + w - 2.65f, z + d - .98f, 1.58f,
                              .22f, .20f, .28f, assets, "HH_A579");
      (void)fittedCatalogMesh(s, f, x + w - 1.72f, z + d - .83f, 1.08f,
                              .72f, .12f, .18f, assets, "HH_A580");
      (void)fittedCatalogMesh(s, f, x + w - 1.72f, z + d - .83f, 1.36f,
                              .72f, .18f, .22f, assets, "HH_A581");
      (void)fittedCatalogMesh(s, f, x + w - 2.10f, z + d - 1.35f, 1.24f,
                              .34f, .24f, .10f, assets, "HH_A582");
      (void)fittedCatalogMesh(s, f, x + w - 1.88f, z + d - 1.35f, 1.31f,
                              .18f, .18f, .14f, assets, "HH_A583");
      (void)fittedCatalogMesh(s, f, x + w - 2.42f, z + d - .83f, 1.18f,
                              .20f, .14f, .30f, assets, "HH_A584");
    }

    bool assetGuestDesk = fittedCatalogMesh(
        s, f, x + 1.6f, z + d - 1.4f, .40f, 1.45f, .56f, .80f,
        assets, "HH_A560");
    if (!assetGuestDesk && assets)
      assetGuestDesk = fittedMesh(s, f, x + 1.6f, z + d - 1.4f, .40f,
                                  1.45f, .56f, .80f, assets->guestDesk);
    if (!assetGuestDesk)
      box(s, f, x + 1.6f, z + d - 1.4f, .73f, 1.45f, .56f, .12f, wood);
    bool assetDeskChair = fittedCatalogMesh(
        s, f, x + 1.6f, z + d - 1.0f, .40f, .50f, .50f, .70f,
        assets, "HH_A561");
    if (!assetDeskChair && assets)
      assetDeskChair = fittedMesh(s, f, x + 1.6f, z + d - 1.0f, .40f,
                                  .50f, .50f, .70f, assets->deskChair);
    if (!assetDeskChair)
      box(s, f, x + 1.6f, z + d - 1.0f, .40f, .5f, .5f, .70f, teal);

    if (assets && w >= 5.0f && d >= 5.0f) {
      if (!fittedCatalogMesh(s, f, x + w - .72f, z + 1.10f, .88f,
                             .76f, .50f, 1.76f, assets,
                             w >= 7.0f ? "HH_A564" : "HH_A563"))
        (void)fittedMesh(s, f, x + w - .72f, z + 1.10f, .88f,
                         .76f, .50f, 1.76f, assets->wardrobe);
      if (!fittedCatalogMesh(s, f, x + w - .42f, z + d * .50f, .36f,
                             .62f, 1.08f, .72f, assets, "HH_A569"))
        (void)fittedMesh(s, f, x + w - .42f, z + d * .50f, .36f,
                         .62f, 1.08f, .72f, assets->tvConsole);
      if (!fittedCatalogMesh(s, f, x + w - .11f, z + d * .50f, 1.46f,
                             .12f, 1.00f, .58f, assets, "HH_A568"))
        (void)fittedMesh(s, f, x + w - .11f, z + d * .50f, 1.46f,
                         .12f, 1.00f, .58f, assets->wallTelevision);
      if (!fittedCatalogMesh(s, f, x + w - 1.25f, z + 3.15f, .44f,
                             .74f, .74f, .88f, assets,
                             w >= 8.0f ? "HH_A593" : "HH_A562"))
        (void)fittedMesh(s, f, x + w - 1.25f, z + 3.15f, .44f,
                         .74f, .74f, .88f, assets->guestArmchair);
      if (d >= 6.0f &&
          !fittedCatalogMesh(s, f, x + 2.0f, z + 4.15f, .24f,
                             1.20f, .46f, .48f, assets, "HH_A565"))
        (void)fittedMesh(s, f, x + 2.0f, z + 4.15f, .24f,
                         1.20f, .46f, .48f, assets->luggageBench);
      (void)fittedCatalogMesh(s, f, x + .72f, z + 1.05f, .56f,
                              .66f, .52f, 1.12f, assets, "HH_A566");
      (void)fittedCatalogMesh(s, f, x + .72f, z + 1.65f, .42f,
                              .58f, .48f, .84f, assets, "HH_A585");
      if (w >= 6.0f && d >= 6.0f) {
        (void)fittedCatalogMesh(s, f, x + .72f, z + 2.28f, .55f,
                                .66f, .52f, 1.10f, assets, "HH_A586");
        (void)fittedCatalogMesh(s, f, x + w * .50f, z + d - .10f, 1.42f,
                                .90f, .10f, .76f, assets, "HH_A570");
        (void)fittedCatalogMesh(s, f, x + .10f, z + d * .52f, 1.20f,
                                .10f, .72f, 1.44f, assets, "HH_A571");
        (void)fittedCatalogMesh(s, f, x + w - 2.02f, z + 3.15f, .27f,
                                .48f, .48f, .54f, assets, "HH_A594");
      }
    }

    plant(s, f, x + w - 1.5f, z + 1.5f, assets);
    box(s, f, static_cast<float>(r.door.x) + .5f,
        static_cast<float>(r.door.y) + .5f, .05f, .7f, .7f, .09f,
        statusColor(r.status));
    if (r.id == c.selected) {
      const Color selected{.97f, .77f, .33f, .75f};
      box(s, f, x + w * .5f, z, .12f, w, .10f, .12f, selected,
          RenderCategory::Selection);
      box(s, f, x + w * .5f, z + d, .12f, w, .10f, .12f, selected,
          RenderCategory::Selection);
      box(s, f, x, z + d * .5f, .12f, .10f, d, .12f, selected,
          RenderCategory::Selection);
      box(s, f, x + w, z + d * .5f, .12f, .10f, d, .12f, selected,
          RenderCategory::Selection);
      s.focusTarget =
          Vec3{x + w * .5f, static_cast<float>(f) * 3.2f + 1, z + d * .5f};
    }
  }

  // Character GLBs are currently rendered in bind pose. Procedural characters
  // remain a fallback for tests, incomplete packages, and future unsupported
  // roles until skeletal animation playback is wired to the renderer.
  for (const auto &p : snapshot.people) {
    if (p.state == PersonState::CheckedOut || p.state == PersonState::OffDuty)
      continue;
    const int f = p.position.floor;
    const float x = static_cast<float>(p.position.x) + .5f,
                z = static_cast<float>(p.position.y) + .5f;
    Color shirt = teal;
    if (p.kind == PersonKind::Guest)
      shirt = (p.id % 2 == 0) ? Color{.75f, .42f, .27f, 1}
                              : Color{.35f, .45f, .66f, 1};
    if (p.kind == PersonKind::Housekeeper)
      shirt = {.70f, .71f, .86f, 1};
    if (p.kind == PersonKind::Maintenance)
      shirt = {.85f, .63f, .22f, 1};
    if (c.overlay == Overlay::GuestSatisfaction && p.kind == PersonKind::Guest)
      shirt = diagnosticGoodness(static_cast<float>(p.satisfaction) / 100.0f);
    if (c.overlay == Overlay::StaffUtilization && p.kind != PersonKind::Guest) {
      const bool active = p.state == PersonState::Working ||
                          p.state == PersonState::Traveling;
      shirt = diagnosticGoodness(active ? 1.0f : 0.0f);
    }
    if (c.overlay == Overlay::QueueWait) {
      const float pressure =
          std::min(1.0f, static_cast<float>(p.queueWaitSeconds) / 300.0f);
      shirt = diagnosticGoodness(1.0f - pressure);
    }

    Color characterTint{1, 1, 1, 1};
    if ((c.overlay == Overlay::GuestSatisfaction &&
         p.kind == PersonKind::Guest) ||
        (c.overlay == Overlay::StaffUtilization &&
         p.kind != PersonKind::Guest) ||
        c.overlay == Overlay::QueueWait) {
      characterTint = shirt;
    }

    box(s, f, x, z, .07f, .49f, .43f, .035f, {.18f, .20f, .17f, .35f});
    if (assets) {
      const bool serviceActive =
          p.state == PersonState::Working || p.state == PersonState::Traveling;
      if (serviceActive && p.kind == PersonKind::Housekeeper) {
        (void)fittedMesh(s, f, x + .55f, z + .34f, .48f,
                         .52f, .66f, .96f, assets->housekeepingCart);
      } else if (serviceActive && p.kind == PersonKind::Maintenance) {
        (void)fittedMesh(s, f, x + .55f, z - .34f, .44f,
                         .62f, .72f, .88f, assets->utilityCart);
      }

      constexpr float halfPi = 1.57079632679f;
      const float yaw =
          static_cast<float>(static_cast<std::size_t>(p.id) % 4u) * halfPi;
      if (fittedMesh(s, f, x, z, .825f, .58f, .46f, 1.65f,
                     characterVisual(*assets, p), characterTint,
                     RenderCategory::Object, yaw))
        continue;
    }

    const float stride =
        !c.reducedMotion && p.state == PersonState::Traveling
            ? .10f *
                  std::sin(static_cast<float>(snapshot.elapsedSeconds % 100) *
                               1.2f +
                           static_cast<float>(p.id % 11))
            : 0;
    box(s, f, x - .11f, z + stride, .30f, .15f, .19f, .48f,
        {.19f, .23f, .27f, 1});
    box(s, f, x + .11f, z - stride, .30f, .15f, .19f, .48f,
        {.19f, .23f, .27f, 1});
    box(s, f, x, z, .79f, .42f, .27f, .55f, shirt);
    box(s, f, x, z, 1.22f, .30f, .29f, .32f, {.78f, .58f, .41f, 1});
    box(s, f, x, z + .02f, 1.39f, .32f, .31f, .12f,
        {.26f, .20f, .16f, 1});
    box(s, f, x - .29f, z, .74f, .13f, .16f, .43f, shirt);
    box(s, f, x + .29f, z, .74f, .13f, .16f, .43f, shirt);
    if (p.kind == PersonKind::Guest)
      box(s, f, x + .43f, z + .15f, .32f, .20f, .30f, .44f,
          {.42f, .27f, .20f, 1});
  }

  if (c.hoverX >= 0 && c.hoverY >= 0 && c.showPreview) {
    const float size = c.previewSize;
    const float w =
        std::min(size, static_cast<float>(snapshot.width - c.hoverX));
    const float d =
        std::min(size, static_cast<float>(snapshot.height - c.hoverY));
    box(s, c.floor, static_cast<float>(c.hoverX) + w * .5f,
        static_cast<float>(c.hoverY) + d * .5f, .14f, w, d, .12f,
        c.previewValid ? Color{.40f, .82f, .67f, .45f}
                       : Color{.88f, .24f, .19f, .55f},
        RenderCategory::Selection);
  }
  return s;
}

RenderScene runtimeAssetCatalogScene(
    const WorldAssetSet &assets,
    std::uint32_t firstAssetNumber,
    std::uint32_t lastAssetNumber) {
  RenderScene scene;
  scene.activeFloor = 0;
  if (firstAssetNumber > lastAssetNumber)
    return scene;

  constexpr std::uint32_t columns = 25u;
  constexpr float spacing = 1.45f;
  for (std::uint32_t number = firstAssetNumber;
       number <= lastAssetNumber; ++number) {
    const std::string assetId = milestoneAssetId(number);
    if (!findWorldAsset(assets, assetId))
      continue;
    const std::uint32_t index = number - firstAssetNumber;
    const float x = static_cast<float>(index % columns) * spacing;
    const float z = static_cast<float>(index / columns) * spacing;
    (void)fittedCatalogMesh(scene, 0, x, z, .65f, .92f, .92f, 1.30f,
                            &assets, assetId);
  }
  return scene;
}
} // namespace hh::client
