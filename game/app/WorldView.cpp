#include "WorldView.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <set>
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

void wallEdge(RenderScene &scene, const GridEdge &edge,
              Color color = {.87f, .82f, .70f, 1},
              RenderCategory category = RenderCategory::Wall) {
  if (edge.axis == EdgeAxis::Vertical)
    box(scene, edge.floor, static_cast<float>(edge.x),
        static_cast<float>(edge.y) + .5f, 1.25f, .12f, .98f, 2.5f,
        color, category);
  else
    box(scene, edge.floor, static_cast<float>(edge.x) + .5f,
        static_cast<float>(edge.y), 1.25f, .98f, .12f, 2.5f,
        color, category);
}

void doorEdge(RenderScene &scene, const GridEdge &edge,
              Color color = wood,
              RenderCategory category = RenderCategory::Wall) {
  if (edge.axis == EdgeAxis::Vertical) {
    box(scene, edge.floor, static_cast<float>(edge.x),
        static_cast<float>(edge.y) + .08f, 1.05f, .12f, .16f, 2.1f, color,
        category);
    box(scene, edge.floor, static_cast<float>(edge.x),
        static_cast<float>(edge.y) + .92f, 1.05f, .12f, .16f, 2.1f, color,
        category);
    box(scene, edge.floor, static_cast<float>(edge.x),
        static_cast<float>(edge.y) + .5f, 2.15f, .12f, .84f, .2f, color,
        category);
  } else {
    box(scene, edge.floor, static_cast<float>(edge.x) + .08f,
        static_cast<float>(edge.y), 1.05f, .16f, .12f, 2.1f, color,
        category);
    box(scene, edge.floor, static_cast<float>(edge.x) + .92f,
        static_cast<float>(edge.y), 1.05f, .16f, .12f, 2.1f, color,
        category);
    box(scene, edge.floor, static_cast<float>(edge.x) + .5f,
        static_cast<float>(edge.y), 2.15f, .84f, .12f, .2f, color, category);
  }
}

void constructionObject(RenderScene &scene, const ConstructionObjectView &object,
                        const WorldAssetSet *assets) {
  if (object.footprint.empty())
    return;
  int minX = object.footprint.front().x;
  int maxX = minX;
  int minY = object.footprint.front().y;
  int maxY = minY;
  for (const Position tile : object.footprint) {
    minX = std::min(minX, tile.x);
    maxX = std::max(maxX, tile.x);
    minY = std::min(minY, tile.y);
    maxY = std::max(maxY, tile.y);
  }
  const float width = static_cast<float>(maxX - minX + 1);
  const float depth = static_cast<float>(maxY - minY + 1);
  const float x = static_cast<float>(minX) + width * .5f;
  const float z = static_cast<float>(minY) + depth * .5f;
  const int floor = object.position.floor;
  const float yaw = static_cast<float>(object.quarterTurns) * 1.57079632679f;
  switch (object.kind) {
  case ConstructionObjectKind::SingleBed:
  case ConstructionObjectKind::DoubleBed: {
    const auto &definition = objectDefinition(object.kind);
    if (assets && fittedMesh(scene, floor, x, z, .58f,
                             static_cast<float>(definition.width) * .86f,
                             static_cast<float>(definition.height) * .86f,
                             1.16f, assets->guestBed,
                             {1, 1, 1, 1}, RenderCategory::Object, yaw))
      return;
    box(scene, floor, x, z, .28f, width * .85f, depth * .85f, .38f, wood);
    box(scene, floor, x, z, .52f, width * .82f, depth * .82f, .23f, cream);
    box(scene, floor, x, z, .66f, width * .77f, depth * .40f, .12f, teal);
    return;
  }
  case ConstructionObjectKind::Toilet:
    if (assets && fittedMesh(scene, floor, x, z, .26f, .48f, .67f, .45f,
                             assets->bathroomToilet))
      return;
    box(scene, floor, x, z, .25f, .55f, .62f, .42f, cream);
    box(scene, floor, x, z - .15f, .52f, .50f, .26f, .18f, cream);
    return;
  case ConstructionObjectKind::Sink:
    if (assets && fittedMesh(scene, floor, x, z, .59f, .65f, .55f, 1.18f,
                             assets->bathroomVanity))
      return;
    box(scene, floor, x, z, .48f, .63f, .50f, .92f, wood);
    box(scene, floor, x, z, .98f, .70f, .57f, .10f, cream);
    return;
  case ConstructionObjectKind::Shower:
  case ConstructionObjectKind::Bath:
    if (object.kind == ConstructionObjectKind::Shower && assets &&
        fittedMesh(scene, floor, x, z, .72f, .10f, .80f, 1.3f,
                   assets->showerGlass, {1, 1, 1, .62f}))
      return;
    box(scene, floor, x, z, .18f, .88f, .88f, .24f,
        {.63f, .79f, .77f, .65f});
    return;
  case ConstructionObjectKind::Light:
    box(scene, floor, x, z, 2.20f, .35f, .35f, .12f, gold);
    box(scene, floor, x, z, 1.94f, .12f, .12f, .42f, wood);
    return;
  case ConstructionObjectKind::Desk:
    if (assets && fittedMesh(scene, floor, x, z, .70f, .85f, .55f, .80f,
                             assets->guestDesk, {1, 1, 1, 1},
                             RenderCategory::Object, yaw))
      return;
    box(scene, floor, x, z, .72f, width * .82f, depth * .62f, .12f, wood);
    return;
  case ConstructionObjectKind::Chair:
    box(scene, floor, x, z, .25f, .62f, .62f, .5f, teal);
    box(scene, floor, x, z - depth * .28f, .64f, .62f, .12f, .78f, wood);
    return;
  case ConstructionObjectKind::Nightstand:
    if (assets && fittedMesh(scene, floor, x, z, .40f, .50f, .52f, .80f,
                             assets->nightstand, {1, 1, 1, 1},
                             RenderCategory::Object, yaw))
      return;
    box(scene, floor, x, z, .40f, .72f, .72f, .8f, wood);
    box(scene, floor, x, z, .84f, .76f, .76f, .10f, gold);
    return;
  case ConstructionObjectKind::Plant:
    plant(scene, floor, x, z, assets);
    return;
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
    if (t.kind == TileKind::Wall || t.kind == TileKind::Door) {
      // The compatibility tile projection marks edge locations; explicit
      // construction snapshots below own all wall and door geometry.
    } else if (t.kind == TileKind::Lobby) {
      box(s, f, x, z, .075f, .88f, .88f, .04f,
          {.18f * shade, .45f * shade, .43f * shade, 1}, RenderCategory::Floor);
      box(s, f, x, z, .10f, .16f, .16f, .05f, gold, RenderCategory::Floor);
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

  for (const auto &edge : snapshot.constructionWalls)
    wallEdge(s, edge);
  for (const auto &edge : snapshot.constructionDoors)
    doorEdge(s, edge);

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
    const std::set<Position> roomTiles(r.tiles.begin(), r.tiles.end());
    for (const Position tile : r.tiles)
      box(s, tile.floor, static_cast<float>(tile.x) + .5f,
          static_cast<float>(tile.y) + .5f, .07f, .82f, .82f, .05f, rug);
    box(s, f, static_cast<float>(r.door.x) + .5f,
        static_cast<float>(r.door.y) + .5f, .05f, .7f, .7f, .09f,
        statusColor(r.status));
    if (r.id == c.selected) {
      const Color selected{.97f, .77f, .33f, .75f};
      const std::array<GridSide, 4> sides{GridSide::North, GridSide::East,
                                          GridSide::South, GridSide::West};
      for (const Position tile : r.tiles)
        for (const GridSide side : sides) {
          const GridEdge edge = edgeForSide(tile, side);
          Position neighbor = tile;
          switch (side) {
          case GridSide::North: --neighbor.y; break;
          case GridSide::East: ++neighbor.x; break;
          case GridSide::South: ++neighbor.y; break;
          case GridSide::West: --neighbor.x; break;
          }
          if (roomTiles.contains(neighbor) || r.primaryDoorEdge == edge)
            continue;
          if (edge.axis == EdgeAxis::Vertical)
            box(s, f, static_cast<float>(edge.x),
                static_cast<float>(edge.y) + .5f, .14f, .08f, .96f, .12f,
                selected, RenderCategory::Selection);
          else
            box(s, f, static_cast<float>(edge.x) + .5f,
                static_cast<float>(edge.y), .14f, .96f, .08f, .12f,
                selected, RenderCategory::Selection);
        }
      s.focusTarget =
          Vec3{x + w * .5f, static_cast<float>(f) * 3.2f + 1, z + d * .5f};
    }
  }

  for (const auto &object : snapshot.constructionObjects)
    constructionObject(s, object, assets);

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
    const Color preview = c.previewValid ? Color{.40f, .82f, .67f, .45f}
                                         : Color{.88f, .24f, .19f, .55f};
    if (c.previewKind == ConstructionPreviewKind::Wall ||
        c.previewKind == ConstructionPreviewKind::Door) {
      const GridEdge edge = edgeForSide(
          {c.floor, c.hoverX, c.hoverY}, c.previewEdgeSide);
      if (c.previewKind == ConstructionPreviewKind::Wall)
        wallEdge(s, edge, preview, RenderCategory::Selection);
      else
        doorEdge(s, edge, preview, RenderCategory::Selection);
    } else if (c.previewKind == ConstructionPreviewKind::Object) {
      const ConstructionObject object{
          0, c.previewObjectKind, {c.floor, c.hoverX, c.hoverY},
          c.previewQuarterTurns};
      const ConstructionObjectView objectView{
          0, c.previewObjectKind, object.anchor, object.quarterTurns,
          footprintTiles(object)};
      const auto itemStart = s.items.size();
      const auto meshStart = s.meshes.size();
      constructionObject(s, objectView, assets);
      for (std::size_t index = itemStart; index < s.items.size(); ++index) {
        s.items[index].color = preview;
        s.items[index].category = RenderCategory::Selection;
      }
      for (std::size_t index = meshStart; index < s.meshes.size(); ++index) {
        s.meshes[index].tint = preview;
        s.meshes[index].translucent = true;
      }
    } else {
      const float size = c.previewKind == ConstructionPreviewKind::Room
                             ? c.previewSize
                             : 1.f;
      const float w =
          std::min(size, static_cast<float>(snapshot.width - c.hoverX));
      const float d =
          std::min(size, static_cast<float>(snapshot.height - c.hoverY));
      box(s, c.floor, static_cast<float>(c.hoverX) + w * .5f,
          static_cast<float>(c.hoverY) + d * .5f, .14f, w, d, .12f, preview,
          RenderCategory::Selection);
    }
  }
  return s;
}
} // namespace hh::client
