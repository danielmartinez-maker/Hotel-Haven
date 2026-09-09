#include "hh/renderer/RuntimeAssetRegistry.h"

#include "hh/assets/Hasset.h"
#include "hh/assets/Types.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <vector>

namespace hh::renderer {
namespace {

std::vector<std::byte> readBinaryFile(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("cannot open cooked asset: " + path.string());
    }

    std::vector<char> chars(
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>());
    if (!input.eof() && input.fail()) {
        throw std::runtime_error("failed reading cooked asset: " + path.string());
    }

    std::vector<std::byte> bytes;
    bytes.reserve(chars.size());
    for (const unsigned char value : chars) {
        bytes.push_back(static_cast<std::byte>(value));
    }
    return bytes;
}

}  // namespace

AssetHandle RuntimeAssetRegistry::addHasset(std::span<const std::byte> bytes) {
    const hh::assets::HassetDocument document = hh::assets::parse_hasset(bytes);
    if (document.type != hh::assets::AssetType::StaticMesh) {
        throw std::runtime_error("runtime mesh registry accepts StaticMesh cooked assets only");
    }
    if (document.asset_id.empty()) {
        throw std::runtime_error("cooked runtime asset has an empty asset_id");
    }
    if (handlesById_.find(document.asset_id) != handlesById_.end()) {
        throw std::runtime_error("duplicate runtime asset id: " + document.asset_id);
    }
    if (assets_.size() > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max())) {
        throw std::runtime_error("runtime asset handle space exhausted");
    }

    // Decode fully before mutating the registry so malformed assets cannot leave
    // a partially registered ID or consume a handle.
    RuntimeAsset candidate;
    candidate.assetId = document.asset_id;
    candidate.mesh = loadGlbMesh(document.payload);

    const AssetHandle handle{static_cast<std::uint32_t>(assets_.size())};
    assets_.push_back(std::move(candidate));
    try {
        const auto [it, inserted] = handlesById_.emplace(assets_.back().assetId, handle);
        static_cast<void>(it);
        if (!inserted) {
            assets_.pop_back();
            throw std::runtime_error("duplicate runtime asset id: " + document.asset_id);
        }
    } catch (...) {
        if (assets_.size() == static_cast<std::size_t>(handle.value) + 1u &&
            handlesById_.find(document.asset_id) == handlesById_.end()) {
            assets_.pop_back();
        }
        throw;
    }
    return handle;
}

void RuntimeAssetRegistry::loadDirectory(const std::filesystem::path& cookedRoot) {
    if (!std::filesystem::exists(cookedRoot) || !std::filesystem::is_directory(cookedRoot)) {
        throw std::runtime_error("cooked asset root is not a directory: " + cookedRoot.string());
    }

    std::vector<std::filesystem::path> paths;
    for (const auto& entry : std::filesystem::directory_iterator(cookedRoot)) {
        if (entry.is_regular_file() && entry.path().extension() == ".hasset") {
            paths.push_back(entry.path());
        }
    }
    std::sort(paths.begin(), paths.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.generic_string() < rhs.generic_string();
    });
    if (paths.empty()) {
        throw std::runtime_error("cooked asset root contains no .hasset files: " + cookedRoot.string());
    }

    // Build a temporary registry first. Directory loading is therefore atomic:
    // one corrupt or duplicate cooked asset leaves the current registry intact.
    RuntimeAssetRegistry staged;
    for (const auto& path : paths) {
        const auto bytes = readBinaryFile(path);
        (void)staged.addHasset(bytes);
    }

    assets_ = std::move(staged.assets_);
    handlesById_ = std::move(staged.handlesById_);
}

AssetHandle RuntimeAssetRegistry::resolve(std::string_view assetId) const {
    const auto it = handlesById_.find(std::string(assetId));
    if (it == handlesById_.end()) {
        throw std::runtime_error("unknown runtime asset id: " + std::string(assetId));
    }
    return it->second;
}

const RuntimeAsset& RuntimeAssetRegistry::asset(AssetHandle handle) const {
    const std::size_t index = static_cast<std::size_t>(handle.value);
    if (index >= assets_.size()) {
        throw std::runtime_error("runtime asset handle is out of range");
    }
    return assets_[index];
}

std::size_t RuntimeAssetRegistry::size() const noexcept {
    return assets_.size();
}

}  // namespace hh::renderer
