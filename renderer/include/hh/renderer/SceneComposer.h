#pragma once

#include "hh/renderer/FloorVisibility.h"
#include "hh/renderer/RenderScene.h"

namespace hh::renderer {

class SceneComposer {
public:
    void compose(
        const RenderScene& scene,
        FloorContextMode contextMode,
        WallRenderMode wallMode,
        Vec3 cameraWorldPosition,
        ComposedScene& result) const;

    [[nodiscard]] ComposedScene compose(
        const RenderScene& scene,
        FloorContextMode contextMode,
        WallRenderMode wallMode,
        Vec3 cameraWorldPosition) const;
};

}  // namespace hh::renderer
