#pragma once

#include "hh/renderer/MathTypes.h"

namespace hh::renderer {

[[nodiscard]] bool segmentIntersectsAabb(
    Vec3 start,
    Vec3 end,
    const Aabb& bounds) noexcept;

}  // namespace hh::renderer
