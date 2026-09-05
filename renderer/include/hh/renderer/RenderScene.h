#pragma once

#include <optional>
#include <vector>

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

struct RenderScene {
    std::vector<BoxRenderItem> items;
    std::optional<Vec3> focusTarget;
    int activeFloor{};
};

struct ComposedBox {
    BoxRenderItem item;
    bool cutaway{};
};

struct ComposedScene {
    std::vector<ComposedBox> opaque;
    std::vector<ComposedBox> translucent;
    std::vector<ComposedBox> wireframe;
};

}  // namespace hh::renderer
