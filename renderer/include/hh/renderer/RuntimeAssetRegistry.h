#pragma once

#include "hh/renderer/AssetHandle.h"
#include "hh/renderer/GlbLoader.h"

#include <cstddef>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace hh::renderer {

struct RuntimeAsset {
    std::string assetId;
    RuntimeMesh mesh;
};

class RuntimeAssetRegistry {
public:
    [[nodiscard]] AssetHandle addHasset(std::span<const std::byte> bytes);
    void loadDirectory(const std::filesystem::path& cookedRoot);

    [[nodiscard]] AssetHandle resolve(std::string_view assetId) const;
    [[nodiscard]] const RuntimeAsset& asset(AssetHandle handle) const;
    [[nodiscard]] std::size_t size() const noexcept;

private:
    std::vector<RuntimeAsset> assets_;
    std::unordered_map<std::string, AssetHandle> handlesById_;
};

}  // namespace hh::renderer
