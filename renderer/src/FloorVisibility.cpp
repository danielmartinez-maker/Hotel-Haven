#include "hh/renderer/FloorVisibility.h"

#include <cstdlib>

namespace hh::renderer {

FloorRenderVisibility floorVisibility(
    int itemFloor,
    int activeFloor,
    FloorContextMode mode) noexcept {
    if (itemFloor == activeFloor) {
        return FloorRenderVisibility::Full;
    }

    if (mode == FloorContextMode::AdjacentContext &&
        std::abs(itemFloor - activeFloor) == 1) {
        return FloorRenderVisibility::TranslucentShell;
    }

    return FloorRenderVisibility::Hidden;
}

}  // namespace hh::renderer
