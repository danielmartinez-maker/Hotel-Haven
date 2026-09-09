#pragma once

#include "WorldView.h"
#include "hh/renderer/RuntimeAssetRegistry.h"

namespace hh::client {

WorldAssetSet worldAssetsFromRegistry(
    const hh::renderer::RuntimeAssetRegistry& registry);

} // namespace hh::client
