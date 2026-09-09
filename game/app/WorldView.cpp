#include "WorldView.h"
#include <algorithm>
#include <array>
#include <cmath>
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

void plant(RenderScene &s, int f, float x, float z,
           const WorldAssetSet *assets) {
  if (assets && fittedMesh(s, f, x, z, .58f, .75f, .70f, 1.16f,
                           assets->pottedPlant))
    return;
  box(s, f, x, z, .20f, .34f, .34f, .4f, {.70f, .42f, .29f, 1});
  box(s, f, x, z, .68f, .70f, .62f, .60f, {.23f, .42f, .25f, 1});
  box(s, f, x + .1f, z, .96f, .45f, .48f, .4f, {.31f, .51f, .29f, 1});
}

void bed(RenderScene &s, int f, float x, float z, const WorldAssetSet *assets) {
  const bool assetBed =
      assets && fittedMesh(s, f, x, z, .58f, 1.62f, 2.10f, 1.16f,
                           assets->guestBed);
  if (!assetBed) {
    box(s, f, x, z, .28f, 1.5f, 2.1f, .38f, wood);
    box(s, f, x, z, .52f, 1.46f, 2.0f, .23f, cream);
    box(s, f, x, z + .3f, .65f, 1.48f, 1.35f, .12f, teal);
    box(s, f, x, z - .72f, .72f, 1.24f, .42f, .17f,
        {.99f, .97f, .88f, 1});
    box(s, f, x, z - 1.05f, .67f, 1.62f, .13f, 1.0f, wood);
  }
  const bool assetNightstand =
      assets && fittedMesh(s, f, x + 1.10f, z - .65f, .40f, .50f, .52f, .80f,
                           assets->nightstand);
  if (!assetNightstand)
    box(s, f, x + 1.10f, z - .65f, .40f, .50f, .52f, .8f, wood);
  box(s, f, x + 1.10f, z - .65f, .98f, .32f, .32f, .28f, gold);
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
} // namespace

RenderScene worldScene(const SimulationView &snapshot, const WorldViewOptions &c,
                       const WorldAssetSet *assets) {
  RenderScene s;
  s.activeFloor = c.floor;
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
    } else if (t.kind == TileKind::Wall) {
      box(s, f, x, z, 1.3f, .97f, .97f, 2.6f, {.87f, .82f, .70f, 1},
          RenderCategory::Wall);
      box(s, f, x, z, .12f, 1.0f, 1.0f, .15f, wood);
    } else if (t.kind == TileKind::Door) {
      box(s, f, x - .43f, z, 1.f, .12f, .22f, 2.f, wood);
      box(s, f, x + .43f, z, 1.f, .12f, .22f, 2.f, wood);
      box(s, f, x, z, 2.02f, .98f, .22f, .13f, wood);
    } else if (t.kind == TileKind::FrontDesk) {
      const bool assetDesk = assets &&
          fittedMesh(s, f, x, z, .70f, 1.73f, .83f, 1.40f,
                     assets->receptionDesk);
      if (!assetDesk) {
        box(s, f, x, z, .52f, 1.6f, .75f, 1.04f, teal);
        box(s, f, x, z, 1.08f, 1.73f, .83f, .13f, cream);
        box(s, f, x + .25f, z, 1.33f, .4f, .12f, .36f,
            {.18f, .21f, .22f, 1});
        box(s, f, x - .40f, z, 1.18f, .14f, .14f, .09f, gold);
      }
    } else if (t.kind == TileKind::SupplyCloset) {
      box(s, f, x, z, .8f, .8f, .75f, 1.6f, wood);
      for (int i = 0; i < 3; ++i)
        box(s, f, x, z - .04f, .30f + static_cast<float>(i) * .48f, .70f,
            .66f, .26f, cream);
    } else if (t.kind == TileKind::Entrance) {
      box(s, f, x, z, .05f, .96f, .96f, .05f, teal);
    } else if (t.kind == TileKind::Stairs) {
      const bool assetStair = assets &&
          fittedMesh(s, f, x, z, .48f, .85f, 1.0f, .96f,
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
    if (c.overlay == Overlay::Cleanliness) {
      const float v = static_cast<float>(r.cleanliness) / 100;
      rug = {.85f - .55f * v, .28f + .4f * v, .22f + .22f * v, 1};
    }
    if (c.overlay == Overlay::Condition) {
      const float v = static_cast<float>(r.condition) / 100;
      rug = {.85f - .55f * v, .28f + .4f * v, .22f + .22f * v, 1};
    }
    box(s, f, x + w * .5f, z + d * .5f, .07f, std::max(1.f, w - 2.1f),
        std::max(1.f, d - 2.1f), .05f, rug);
    bed(s, f, x + 2.f, z + 2.5f, assets);
    if (r.beds > 1 && w >= 8)
      bed(s, f, x + w - 2.f, z + 2.5f, assets);

    // Bathroom fixtures remain presentation-only; room simulation owns baths.
    if (r.baths > 0) {
      box(s, f, x + w - 1.7f, z + d - 1.6f, .09f, 1.7f, 1.6f, .07f,
          {.80f, .88f, .84f, 1});
      const bool assetToilet = assets &&
          fittedMesh(s, f, x + w - 1.35f, z + d - 1.5f, .26f, .48f, .67f,
                     .45f, assets->bathroomToilet);
      if (!assetToilet)
        box(s, f, x + w - 1.35f, z + d - 1.5f, .26f, .48f, .67f, .45f,
            cream);
      box(s, f, x + w - 1.35f, z + d - 1.75f, .61f, .5f, .22f, .5f,
          cream);
      const bool assetVanity = assets &&
          fittedMesh(s, f, x + w - 2.1f, z + d - 1.35f, .59f, .65f, .55f,
                     1.18f, assets->bathroomVanity);
      if (!assetVanity) {
        box(s, f, x + w - 2.1f, z + d - 1.35f, .57f, .56f, .49f, 1.1f,
            wood);
        box(s, f, x + w - 2.1f, z + d - 1.35f, 1.15f, .65f, .55f, .09f,
            cream);
      }
      const bool assetShower = assets &&
          fittedMesh(s, f, x + w - 2.65f, z + d - 1.75f, .72f, .10f, 1.4f,
                     1.3f, assets->showerGlass,
                     {1.f, 1.f, 1.f, .62f});
      if (!assetShower)
        box(s, f, x + w - 2.65f, z + d - 1.75f, .72f, .10f, 1.4f, 1.3f,
            {.63f, .79f, .77f, .55f});
    }

    const bool assetGuestDesk = assets &&
        fittedMesh(s, f, x + 1.6f, z + d - 1.4f, .40f, 1.45f, .56f, .80f,
                   assets->guestDesk);
    if (!assetGuestDesk)
      box(s, f, x + 1.6f, z + d - 1.4f, .73f, 1.45f, .56f, .12f, wood);
    box(s, f, x + 1.6f, z + d - 1.0f, .40f, .5f, .5f, .70f, teal);
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

  // Characters remain procedural until the skinned-mesh/animation runtime exists.
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
    const float stride =
        p.state == PersonState::Traveling
            ? .10f *
                  std::sin(static_cast<float>(snapshot.elapsedSeconds % 100) *
                               1.2f +
                           static_cast<float>(p.id % 11))
            : 0;
    box(s, f, x, z, .07f, .49f, .43f, .035f, {.18f, .20f, .17f, .35f});
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
} // namespace hh::client
