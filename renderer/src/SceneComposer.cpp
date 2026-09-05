#include "hh/renderer/SceneComposer.h"

#include "hh/renderer/Occlusion.h"

namespace hh::renderer {

ComposedScene SceneComposer::compose(
    const RenderScene& scene,
    FloorContextMode contextMode,
    WallRenderMode wallMode,
    Vec3 cameraWorldPosition) const {
    ComposedScene result;
    result.opaque.reserve(scene.items.size());
    result.translucent.reserve(scene.items.size());
    result.wireframe.reserve(scene.items.size());

    for (const BoxRenderItem& sourceItem : scene.items) {
        const FloorRenderVisibility visibility =
            floorVisibility(sourceItem.floorId, scene.activeFloor, contextMode);
        if (visibility == FloorRenderVisibility::Hidden) {
            continue;
        }

        ComposedBox composed{sourceItem, false};
        if (visibility == FloorRenderVisibility::TranslucentShell) {
            composed.item.color.a *= 0.25f;
        }

        if (sourceItem.category == RenderCategory::Wall) {
            if (wallMode == WallRenderMode::Blueprint) {
                result.wireframe.push_back(composed);
                continue;
            }

            if (wallMode == WallRenderMode::Cutaway) {
                composed.cutaway = true;
            } else if (scene.focusTarget.has_value() &&
                       segmentIntersectsAabb(cameraWorldPosition,
                                             *scene.focusTarget,
                                             sourceItem.bounds)) {
                composed.cutaway = true;
            }
        }

        if (visibility == FloorRenderVisibility::TranslucentShell) {
            result.translucent.push_back(composed);
        } else {
            result.opaque.push_back(composed);
        }
    }

    return result;
}

}  // namespace hh::renderer
