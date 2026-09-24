#include "hh/renderer/SceneComposer.h"

#include "hh/renderer/Occlusion.h"

namespace hh::renderer {

void SceneComposer::compose(
    const RenderScene& scene,
    FloorContextMode contextMode,
    WallRenderMode wallMode,
    Vec3 cameraWorldPosition,
    ComposedScene& result) const {
    result.opaque.clear();
    result.translucent.clear();
    result.wireframe.clear();
    result.opaqueMeshes.clear();
    result.translucentMeshes.clear();
    result.wireframeMeshes.clear();

    if (result.opaque.capacity() < scene.items.size())
        result.opaque.reserve(scene.items.size());
    if (result.opaqueMeshes.capacity() < scene.meshes.size())
        result.opaqueMeshes.reserve(scene.meshes.size());

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

    for (const MeshRenderItem& sourceItem : scene.meshes) {
        const FloorRenderVisibility visibility =
            floorVisibility(sourceItem.floorId, scene.activeFloor, contextMode);
        if (visibility == FloorRenderVisibility::Hidden) {
            continue;
        }

        ComposedMesh composed{sourceItem, false};
        if (visibility == FloorRenderVisibility::TranslucentShell) {
            composed.item.tint.a *= 0.25f;
        }

        if (sourceItem.category == RenderCategory::Wall) {
            if (wallMode == WallRenderMode::Blueprint) {
                result.wireframeMeshes.push_back(composed);
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

        if (visibility == FloorRenderVisibility::TranslucentShell || sourceItem.translucent) {
            result.translucentMeshes.push_back(composed);
        } else {
            result.opaqueMeshes.push_back(composed);
        }
    }

}

ComposedScene SceneComposer::compose(
    const RenderScene& scene,
    FloorContextMode contextMode,
    WallRenderMode wallMode,
    Vec3 cameraWorldPosition) const {
    ComposedScene result;
    compose(scene, contextMode, wallMode, cameraWorldPosition, result);
    return result;
}

}  // namespace hh::renderer
