#pragma once

#include "hh/renderer/Camera.h"
#include "hh/renderer/RenderScene.h"

namespace hh::renderer {

[[nodiscard]] ComposedScene prepareVisibleScene(
    const ComposedScene& scene,
    const OrthoCamera& camera);

}  // namespace hh::renderer
