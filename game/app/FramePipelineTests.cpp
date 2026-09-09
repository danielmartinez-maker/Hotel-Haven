#include "FramePipeline.h"

#include "hh/renderer/Camera.h"
#include "hh/renderer/RenderScene.h"
#include "hh/renderer/SceneComposer.h"

#include <iostream>
#include <stdexcept>

namespace {

void require(bool value, const char* message) {
  if (!value)
    throw std::runtime_error(message);
}

hh::renderer::BoxRenderItem box(float x, float z) {
  hh::renderer::BoxRenderItem item;
  item.floorId = 0;
  item.center = {x, 0.5f, z};
  item.size = {1.0f, 1.0f, 1.0f};
  item.color = {1, 1, 1, 1};
  item.category = hh::renderer::RenderCategory::Object;
  item.bounds = {{x - 0.5f, 0.0f, z - 0.5f},
                 {x + 0.5f, 1.0f, z + 0.5f}};
  return item;
}

} // namespace

int main() {
  try {
    hh::renderer::RenderScene scene;
    scene.activeFloor = 0;
    scene.items.push_back(box(0.0f, 0.0f));
    scene.items.push_back(box(200.0f, 200.0f));

    hh::renderer::OrthoCamera camera;
    camera.setTarget({0.0f, 0.0f, 0.0f});
    camera.setAspectRatio(1.0f);
    camera.setOrthoHeight(20.0f);
    hh::renderer::SceneComposer composer;

    const auto frame = hh::client::composeVisibleFrame(
        scene, composer, hh::renderer::FloorContextMode::Normal,
        hh::renderer::WallRenderMode::Cutaway, camera);

    require(frame.opaque.size() == 1u,
            "game frame pipeline did not cull off-camera geometry");
    require(frame.opaque.front().item.center.x == 0.0f,
            "game frame pipeline kept the wrong box after culling");
    std::cout << "Game frame visibility pipeline passed\n";
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
