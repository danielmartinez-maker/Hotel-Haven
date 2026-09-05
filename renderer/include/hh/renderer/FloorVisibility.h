#pragma once

namespace hh::renderer {

enum class FloorContextMode {
    Normal,
    AdjacentContext,
};

enum class FloorRenderVisibility {
    Hidden,
    Full,
    TranslucentShell,
};

[[nodiscard]] FloorRenderVisibility floorVisibility(
    int itemFloor,
    int activeFloor,
    FloorContextMode mode) noexcept;

}  // namespace hh::renderer
