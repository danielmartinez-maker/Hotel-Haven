#pragma once

#include "hh/renderer/Camera.h"
#include "hh/renderer/RenderScene.h"

namespace hh::renderer {

void prepareVisibleScene(
    const ComposedScene& scene,
    const OrthoCamera& camera,
    ComposedScene& visible);

[[nodiscard]] ComposedScene prepareVisibleScene(
    const ComposedScene& scene,
    const OrthoCamera& camera);

}  // namespace hh::renderer
