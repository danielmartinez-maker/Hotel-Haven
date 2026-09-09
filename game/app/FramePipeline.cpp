#include "FramePipeline.h"

#include "hh/renderer/Visibility.h"

namespace hh::client {

hh::renderer::ComposedScene composeVisibleFrame(
    const hh::renderer::RenderScene& scene,
    const hh::renderer::SceneComposer& composer,
    hh::renderer::FloorContextMode floorContext,
    hh::renderer::WallRenderMode wallMode,
    const hh::renderer::OrthoCamera& camera) {
  const auto composed = composer.compose(
      scene, floorContext, wallMode, camera.worldPosition());
  return hh::renderer::prepareVisibleScene(composed, camera);
}

} // namespace hh::client
