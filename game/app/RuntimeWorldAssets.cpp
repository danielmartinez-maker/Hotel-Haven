#include "RuntimeWorldAssets.h"

#include <algorithm>

namespace hh::client {

WorldAssetSet worldAssetsFromRegistry(
    const hh::renderer::RuntimeAssetRegistry& registry) {
  return resolveWorldAssets([&registry](std::string_view assetId) {
    const hh::renderer::AssetHandle handle = registry.resolve(assetId);
    const hh::renderer::RuntimeAsset& asset = registry.asset(handle);
    const bool translucent = std::any_of(
        asset.mesh.materials.begin(), asset.mesh.materials.end(),
        [](const hh::renderer::MeshMaterial& material) {
          return material.translucent || material.baseColor.a < 0.999f;
        });
    return WorldAssetVisual{handle, asset.mesh.bounds, translucent};
  });
}

WorldAssetSet loadWorldAssetsFromDirectory(
    hh::renderer::RuntimeAssetRegistry& registry,
    const std::filesystem::path& cookedRoot) {
  registry.loadDirectory(cookedRoot);
  return worldAssetsFromRegistry(registry);
}

} // namespace hh::client
