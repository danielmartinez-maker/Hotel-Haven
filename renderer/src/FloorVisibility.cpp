#include "hh/renderer/FloorVisibility.h"

namespace hh::renderer {

FloorRenderVisibility floorVisibility(
    int itemFloor,
    int activeFloor,
    FloorContextMode mode) noexcept {
    if (itemFloor == activeFloor) {
        return FloorRenderVisibility::Full;
    }

    if (mode == FloorContextMode::AdjacentContext) {
        const long long delta = static_cast<long long>(itemFloor) -
                                static_cast<long long>(activeFloor);
        if (delta == -1LL || delta == 1LL) {
            return FloorRenderVisibility::TranslucentShell;
        }
    }

    return FloorRenderVisibility::Hidden;
}

}  // namespace hh::renderer
