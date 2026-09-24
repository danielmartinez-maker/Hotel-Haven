#pragma once

#include "hh/assets/Types.h"
#include "hh/renderer/AssetHandle.h"
#include "hh/renderer/GlbLoader.h"

#include <cstddef>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace hh::renderer {

struct RuntimeAsset {
    std::string assetId;
    hh::assets::AssetType assetType{hh::assets::AssetType::StaticMesh};
    RuntimeMesh mesh;
};

class RuntimeAssetRegistry {
public:
    [[nodiscard]] AssetHandle addHasset(std::span<const std::byte> bytes);

    // Full package load used by release audits and tooling.
    void loadDirectory(const std::filesystem::path& cookedRoot);

    // Shipping-client load path. Only the named assets are decoded, while still
    // publishing atomically so a missing/corrupt required asset cannot leave a
    // partially replaced registry.
    void loadDirectorySubset(
        const std::filesystem::path& cookedRoot,
        std::span<const std::string_view> requiredAssetIds);

    // Milestone/package load path. Loads every matching cooked asset whose
    // numeric suffix falls inside the inclusive range, while still requiring
    // the supplied core IDs. Assets outside the milestone stay unloaded.
    void loadDirectoryAssetRange(
        const std::filesystem::path& cookedRoot,
        std::string_view assetIdPrefix,
        std::uint32_t firstAssetNumber,
        std::uint32_t lastAssetNumber,
        std::span<const std::string_view> requiredAssetIds = {});

    [[nodiscard]] AssetHandle resolve(std::string_view assetId) const;
    [[nodiscard]] std::optional<AssetHandle> tryResolve(std::string_view assetId) const noexcept;
    [[nodiscard]] bool contains(std::string_view assetId) const noexcept;
    [[nodiscard]] const RuntimeAsset& asset(AssetHandle handle) const;
    [[nodiscard]] std::span<const RuntimeAsset> assets() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;

private:
    void loadFiles(std::span<const std::filesystem::path> paths);

    std::vector<RuntimeAsset> assets_;
    std::unordered_map<std::string, AssetHandle> handlesById_;
};

}  // namespace hh::renderer
