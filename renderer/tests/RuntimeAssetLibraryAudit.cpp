#include "hh/assets/Types.h"
#include "hh/assets/Json.h"
#include "hh/renderer/AssetHandle.h"
#include "hh/renderer/RuntimeAssetRegistry.h"

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
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

hh::assets::JsonValue readJson(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("cannot read asset manifest: " + path.string());
    }
    return hh::assets::parse_json(std::string(std::istreambuf_iterator<char>{input}, {}));
}

std::map<std::string, std::string> expectedTypes(const std::filesystem::path& repoRoot) {
    const auto manifest = readJson(repoRoot / "GameData/AssetDefinitions/hotel_haven_asset_manifest_v2.json");
    const auto& profiles = manifest.at("profiles");
    std::map<std::string, std::string> types;
    std::size_t batchNumber = 0;
    for (const auto& entry : manifest.at("batches").as_array()) {
        ++batchNumber;
        if (static_cast<std::size_t>(entry.at("batch").as_number()) != batchNumber ||
            entry.at("asset_count").as_number() != 50.0) {
            throw std::runtime_error("invalid batch declaration in active manifest");
        }
        const auto shard = readJson(repoRoot / entry.at("path").as_string());
        std::size_t rows = 0;
        for (const auto& group : shard.at("groups").as_array()) {
            for (const auto& row : group.at("assets").as_array()) {
                const auto& fields = row.as_array();
                const auto& id = fields.at(0).as_string();
                const auto& profile = fields.at(4).as_string();
                const auto& kind = profiles.at(profile).at("asset_type").as_string();
                if (!types.emplace(id, kind).second) {
                    throw std::runtime_error("duplicate manifest asset: " + id);
                }
                ++rows;
            }
        }
        if (rows != 50) {
            throw std::runtime_error("batch has a noncanonical row count");
        }
    }
    if (types.size() != static_cast<std::size_t>(manifest.at("asset_count").as_number())) {
        throw std::runtime_error("active manifest count differs from declared assets");
    }
    return types;
}

}  // namespace

int main(int argc, char** argv) {
    try {
        if (argc != 3) {
            throw std::runtime_error(
                "usage: hh_runtime_asset_library_audit <cooked-asset-root> <repo-root>");
        }

        const auto expected = expectedTypes(std::filesystem::path(argv[2]));

        hh::renderer::RuntimeAssetRegistry registry;
        registry.loadDirectory(std::filesystem::path(argv[1]));
        if (registry.size() != expected.size()) {
            throw std::runtime_error(
                "expected exactly " + std::to_string(expected.size()) + " cooked gameplay assets, loaded " +
                std::to_string(registry.size()));
        }

        std::size_t staticMeshes = 0;
        std::size_t skinnedMeshes = 0;
        std::size_t prefabs = 0;
        for (std::size_t index = 0; index < registry.size(); ++index) {
            const auto handle = hh::renderer::AssetHandle{
                static_cast<std::uint32_t>(index)};
            const auto& asset = registry.asset(handle);
            const auto expectedType = expected.find(asset.assetId);
            if (expectedType == expected.end() ||
                asset.assetType != hh::assets::asset_type_from_string(expectedType->second)) {
                throw std::runtime_error(asset.assetId + " has no matching manifest type");
            }
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
            case hh::assets::AssetType::Prefab:
                ++prefabs;
                break;
            default:
                throw std::runtime_error(
                    asset.assetId + " has a non-renderable cooked asset type");
            }
        }

        std::size_t expectedStatic = 0, expectedSkinned = 0, expectedPrefabs = 0;
        for (const auto& [id, kind] : expected) {
            (void)id;
            expectedStatic += kind == "StaticMeshAsset";
            expectedSkinned += kind == "SkinnedMeshAsset";
            expectedPrefabs += kind == "PrefabAsset";
        }
        if (staticMeshes != expectedStatic || skinnedMeshes != expectedSkinned || prefabs != expectedPrefabs) {
            throw std::runtime_error(
                "cooked asset type totals differ from active manifest: loaded " +
                std::to_string(staticMeshes) + " static, " +
                std::to_string(skinnedMeshes) + " skinned and " +
                std::to_string(prefabs) + " prefabs");
        }

        std::cout << "Loaded and validated " << registry.size() << " cooked renderer assets ("
                  << staticMeshes << " static, " << skinnedMeshes << " skinned, "
                  << prefabs << " prefabs)\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
