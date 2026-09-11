#pragma once

#include "hh/renderer/MathTypes.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace hh::renderer {

struct MeshVertex {
    Vec3 position{};
    Vec3 normal{0.0f, 1.0f, 0.0f};
};

struct MeshMaterial {
    Color baseColor{};
    float metallic{1.0f};
    float roughness{1.0f};
    bool translucent{false};
};

struct MeshPrimitive {
    std::vector<MeshVertex> vertices;
    std::vector<std::uint32_t> indices;
    std::size_t materialIndex{};
};

struct RuntimeMesh {
    std::vector<MeshPrimitive> primitives;
    std::vector<MeshMaterial> materials;
    Aabb bounds{};
};

// Decodes a binary glTF 2.0 payload from Hotel Haven's Z-up asset space into
// renderer-native Y-up, left-handed geometry. Scene-node transforms are
// flattened before the asset-to-renderer basis conversion.
// Unsupported or malformed geometry fails fast with std::runtime_error.
[[nodiscard]] RuntimeMesh loadGlbMesh(std::span<const std::byte> bytes);

}  // namespace hh::renderer
