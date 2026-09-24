#pragma once

#include "hh/renderer/Camera.h"
#include "hh/renderer/RenderScene.h"
#include "hh/renderer/SceneComposer.h"

namespace hh::client {

void composeVisibleFrame(
    const hh::renderer::RenderScene& scene,
    const hh::renderer::SceneComposer& composer,
    hh::renderer::FloorContextMode floorContext,
    hh::renderer::WallRenderMode wallMode,
    const hh::renderer::OrthoCamera& camera,
    hh::renderer::ComposedScene& composedScratch,
    hh::renderer::ComposedScene& visibleOut);

[[nodiscard]] hh::renderer::ComposedScene composeVisibleFrame(
    const hh::renderer::RenderScene& scene,
    const hh::renderer::SceneComposer& composer,
    hh::renderer::FloorContextMode floorContext,
    hh::renderer::WallRenderMode wallMode,
    const hh::renderer::OrthoCamera& camera);

} // namespace hh::client
