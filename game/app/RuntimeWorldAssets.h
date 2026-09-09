#pragma once

#include "WorldView.h"
#include "hh/renderer/RuntimeAssetRegistry.h"

#include <filesystem>

namespace hh::client {

WorldAssetSet worldAssetsFromRegistry(
    const hh::renderer::RuntimeAssetRegistry& registry);

WorldAssetSet loadWorldAssetsFromDirectory(
    hh::renderer::RuntimeAssetRegistry& registry,
    const std::filesystem::path& cookedRoot);

} // namespace hh::client
