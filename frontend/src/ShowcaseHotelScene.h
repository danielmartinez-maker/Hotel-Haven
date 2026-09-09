#pragma once

#include "hh/renderer/RenderScene.h"

namespace hh::frontend {

class ShowcaseHotelScene {
public:
    [[nodiscard]] static hh::renderer::RenderScene build();
    static void updateAmbient(hh::renderer::RenderScene& scene, float elapsedSeconds) noexcept;
};

}  // namespace hh::frontend
