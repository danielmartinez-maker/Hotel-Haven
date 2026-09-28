#include "FramePipeline.h"

#include "hh/renderer/Visibility.h"

namespace hh::client {

void composeVisibleFrame(
    const hh::renderer::RenderScene& scene,
    const hh::renderer::SceneComposer& composer,
    hh::renderer::FloorContextMode floorContext,
    hh::renderer::WallRenderMode wallMode,
    const hh::renderer::OrthoCamera& camera,
    hh::renderer::ComposedScene& composedScratch,
    hh::renderer::ComposedScene& visibleOut) {
  composer.compose(scene, floorContext, wallMode, camera.worldPosition(),
                   composedScratch);
  hh::renderer::prepareVisibleScene(composedScratch, camera, visibleOut);
}

hh::renderer::ComposedScene composeVisibleFrame(
    const hh::renderer::RenderScene& scene,
    const hh::renderer::SceneComposer& composer,
    hh::renderer::FloorContextMode floorContext,
    hh::renderer::WallRenderMode wallMode,
    const hh::renderer::OrthoCamera& camera) {
  hh::renderer::ComposedScene composed;
  hh::renderer::ComposedScene visible;
  composeVisibleFrame(scene, composer, floorContext, wallMode, camera,
                      composed, visible);
  return visible;
}

} // namespace hh::client
