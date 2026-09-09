#pragma once

#include <optional>
#include <vector>

#include "hh/renderer/AssetHandle.h"
#include "hh/renderer/MathTypes.h"

namespace hh::renderer {

enum class RenderCategory {
    Floor,
    Wall,
    Object,
    Selection,
};

enum class WallRenderMode {
    FullHeight,
    Cutaway,
    Blueprint,
};

struct BoxRenderItem {
    Vec3 center{};
    Vec3 size{1.0f, 1.0f, 1.0f};
    Color color{1.0f, 1.0f, 1.0f, 1.0f};
    int floorId{};
    RenderCategory category{RenderCategory::Object};
    Aabb bounds{};
};

struct MeshTransform {
    Vec3 translation{};
    Vec3 scale{1.0f, 1.0f, 1.0f};
    float yawRadians{};
};

struct MeshRenderItem {
    AssetHandle asset{};
    MeshTransform transform{};
    Color tint{1.0f, 1.0f, 1.0f, 1.0f};
    int floorId{};
    RenderCategory category{RenderCategory::Object};
    Aabb bounds{};
    bool translucent{};
};

struct RenderScene {
    std::vector<BoxRenderItem> items;
    std::vector<MeshRenderItem> meshes;
    std::optional<Vec3> focusTarget;
    int activeFloor{};
};

struct ComposedBox {
    BoxRenderItem item;
    bool cutaway{};
};

struct ComposedMesh {
    MeshRenderItem item;
    bool cutaway{};
};

struct ComposedScene {
    std::vector<ComposedBox> opaque;
    std::vector<ComposedBox> translucent;
    std::vector<ComposedBox> wireframe;
    std::vector<ComposedMesh> opaqueMeshes;
    std::vector<ComposedMesh> translucentMeshes;
    std::vector<ComposedMesh> wireframeMeshes;
};

}  // namespace hh::renderer
