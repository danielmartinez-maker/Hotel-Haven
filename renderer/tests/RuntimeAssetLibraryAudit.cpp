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

std::size_t parseExpectedCount(const char* value, const char* label) {
    try {
        const std::string text(value);
        std::size_t consumed = 0;
        const auto parsed = std::stoull(text, &consumed);
        if (consumed != text.size()) {
            throw std::runtime_error("trailing characters");
        }
        return static_cast<std::size_t>(parsed);
    } catch (const std::exception&) {
        throw std::runtime_error(std::string("invalid ") + label + " count: " + value);
    }
}

}  // namespace

int main(int argc, char** argv) {
    try {
        if (argc != 2 && argc != 4) {
            throw std::runtime_error(
                "usage: hh_runtime_asset_library_audit <cooked-asset-root> "
                "[expected-static expected-skinned]");
        }

        hh::renderer::RuntimeAssetRegistry registry;
        registry.loadDirectory(std::filesystem::path(argv[1]));
        if (registry.size() == 0u) {
            throw std::runtime_error("runtime asset library is empty");
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

        if (argc == 4) {
            const auto expectedStatic = parseExpectedCount(argv[2], "static mesh");
            const auto expectedSkinned = parseExpectedCount(argv[3], "skinned mesh");
            if (staticMeshes != expectedStatic || skinnedMeshes != expectedSkinned) {
                throw std::runtime_error(
                    "expected " + std::to_string(expectedStatic) + " StaticMesh and " +
                    std::to_string(expectedSkinned) + " SkinnedMesh assets, loaded " +
                    std::to_string(staticMeshes) + " static and " +
                    std::to_string(skinnedMeshes) + " skinned");
            }
        }

        std::cout << "Loaded and validated " << registry.size()
                  << " cooked renderer assets (" << staticMeshes << " static, "
                  << skinnedMeshes << " skinned bind-pose)\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
