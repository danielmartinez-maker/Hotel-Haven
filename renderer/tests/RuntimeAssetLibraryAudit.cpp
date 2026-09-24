#include "hh/assets/Types.h"
#include "hh/renderer/AssetHandle.h"
#include "hh/renderer/RuntimeAssetRegistry.h"

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

bool finiteBounds(const hh::renderer::Aabb& bounds) noexcept {
    return std::isfinite(bounds.min.x) && std::isfinite(bounds.min.y) &&
           std::isfinite(bounds.min.z) && std::isfinite(bounds.max.x) &&
           std::isfinite(bounds.max.y) && std::isfinite(bounds.max.z) &&
           bounds.max.x > bounds.min.x && bounds.max.y > bounds.min.y &&
           bounds.max.z > bounds.min.z;
}

}  // namespace

int main(int argc, char** argv) {
    try {
        if (argc != 2) {
            throw std::runtime_error(
                "usage: hh_runtime_asset_library_audit <cooked-asset-root>");
        }

        hh::renderer::RuntimeAssetRegistry registry;
        registry.loadDirectory(std::filesystem::path(argv[1]));
        if (registry.size() == 0u) {
            throw std::runtime_error("runtime asset registry loaded no cooked meshes");
        }

        std::size_t staticMeshes = 0;
        std::size_t skinnedMeshes = 0;
        for (std::size_t index = 0; index < registry.size(); ++index) {
            const auto handle = hh::renderer::AssetHandle{
                static_cast<std::uint32_t>(index)};
            const auto& asset = registry.asset(handle);
            if (!finiteBounds(asset.mesh.bounds)) {
                throw std::runtime_error(
                    asset.assetId + " has non-finite or empty renderer bounds");
            }
            if (asset.mesh.primitives.empty()) {
                throw std::runtime_error(
                    asset.assetId + " has no drawable mesh primitives");
            }
            for (const auto& primitive : asset.mesh.primitives) {
                if (primitive.vertices.empty() || primitive.indices.empty() ||
                    (primitive.indices.size() % 3u) != 0u) {
                    throw std::runtime_error(
                        asset.assetId + " has invalid drawable triangle geometry");
                }
            }

            switch (asset.assetType) {
            case hh::assets::AssetType::StaticMesh:
                ++staticMeshes;
                break;
            case hh::assets::AssetType::SkinnedMesh:
                ++skinnedMeshes;
                break;
            default:
                throw std::runtime_error(
                    asset.assetId + " has a non-renderable cooked asset type");
            }
        }

        if (staticMeshes + skinnedMeshes != registry.size()) {
            throw std::runtime_error(
                "renderer audit did not classify every cooked runtime asset");
        }

        std::cout << "Loaded and validated " << registry.size()
                  << " cooked renderer assets (" << staticMeshes
                  << " static, " << skinnedMeshes << " skinned bind-pose)\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
