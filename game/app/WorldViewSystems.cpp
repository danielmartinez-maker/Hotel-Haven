#include "WorldView.h"
#include <algorithm>
#include <string_view>

namespace hh::client {
using namespace hh::game;
using namespace hh::renderer;
namespace {
void appendBox(RenderScene &scene, int floor, float x, float z, float y,
               float width, float depth, float height, Color color,
               RenderCategory category = RenderCategory::Object) {
  BoxRenderItem item;
  item.floorId = floor;
  item.center = {x, static_cast<float>(floor) * 3.2f + y, z};
  item.size = {std::max(.02f, width), std::max(.02f, height),
               std::max(.02f, depth)};
  item.color = color;
  item.category = category;
  const auto half = item.size * .5f;
  item.bounds = {item.center - half, item.center + half};
  scene.items.push_back(item);
}

struct ObjectStyle {
  float height{.65f};
  float y{.34f};
  Color color{.43f, .34f, .25f, 1.f};
};

ObjectStyle objectStyle(std::string_view typeId) {
  if (typeId == "chair")
    return {.75f, .39f, {.53f, .36f, .22f, 1.f}};
  if (typeId == "desk")
    return {.82f, .43f, {.28f, .45f, .40f, 1.f}};
  if (typeId == "guest_bed")
    return {.52f, .28f, {.88f, .82f, .68f, 1.f}};
  if (typeId == "wall_sconce")
    return {.34f, 1.55f, {.94f, .72f, .30f, 1.f}};
  if (typeId == "power_source")
    return {1.15f, .59f, {.26f, .49f, .63f, 1.f}};
  if (typeId == "water_source")
    return {1.15f, .59f, {.27f, .62f, .70f, 1.f}};
  if (typeId == "fire_alarm")
    return {.28f, 1.65f, {.82f, .23f, .20f, 1.f}};
  if (typeId == "security_camera")
    return {.25f, 1.75f, {.25f, .28f, .31f, 1.f}};
  if (typeId == "passenger_elevator")
    return {2.35f, 1.18f, {.39f, .45f, .48f, 1.f}};
  if (typeId == "service_elevator")
    return {2.35f, 1.18f, {.48f, .43f, .34f, 1.f}};
  return {};
}

const RoomSystemSnapshot *roomSystem(const BuildingSystemsSnapshot &systems,
                                     EntityId roomId) {
  const auto it = std::find_if(
      systems.rooms.begin(), systems.rooms.end(),
      [&](const RoomSystemSnapshot &system) { return system.roomId == roomId; });
  return it == systems.rooms.end() ? nullptr : &*it;
}
} // namespace

RenderScene worldSceneWithSystems(const SimulationView &snapshot,
                                  const WorldViewOptions &options) {
  RenderScene scene = worldScene(snapshot, options);

  if (options.construction) {
    for (const auto &object : options.construction->objects) {
      const auto style = objectStyle(object.typeId);
      const float width = std::max(.2f, static_cast<float>(object.width) * .82f);
      const float depth =
          std::max(.2f, static_cast<float>(object.height) * .82f);
      const float x = static_cast<float>(object.origin.x) +
                      static_cast<float>(object.width) * .5f;
      const float z = static_cast<float>(object.origin.y) +
                      static_cast<float>(object.height) * .5f;
      appendBox(scene, object.origin.floor, x, z, style.y, width, depth,
                style.height, style.color);
    }
  }

  if (options.buildingSystems &&
      (options.overlay == Overlay::Utilities ||
       options.overlay == Overlay::Egress)) {
    for (const auto &room : snapshot.rooms) {
      const auto *system = roomSystem(*options.buildingSystems, room.id);
      bool valid = false;
      if (system) {
        if (options.overlay == Overlay::Utilities)
          valid = system->powerConnected && system->waterConnected;
        else
          valid = system->egress && system->accessible;
      }
      const Color diagnostic = valid ? Color{.24f, .68f, .43f, .38f}
                                     : Color{.86f, .25f, .20f, .48f};
      appendBox(scene, room.floor,
                static_cast<float>(room.x) +
                    static_cast<float>(room.width) * .5f,
                static_cast<float>(room.y) +
                    static_cast<float>(room.height) * .5f,
                .13f, std::max(.5f, static_cast<float>(room.width) - .3f),
                std::max(.5f, static_cast<float>(room.height) - .3f), .035f,
                diagnostic, RenderCategory::Selection);
    }
  }

  return scene;
}
} // namespace hh::client
