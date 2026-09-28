#include "RuntimeWorldAssets.h"

#include <algorithm>
#include <vector>

namespace hh::client {

WorldAssetVisual visualFromRuntimeAsset(
    hh::renderer::AssetHandle handle,
    const hh::renderer::RuntimeAsset& asset) {
  const bool translucent = std::any_of(
      asset.mesh.materials.begin(), asset.mesh.materials.end(),
      [](const hh::renderer::MeshMaterial& material) {
        return material.translucent || material.baseColor.a < 0.999f;
      });
  return WorldAssetVisual{handle, asset.mesh.bounds, translucent};
}

WorldAssetSet worldAssetsFromRegistry(
    const hh::renderer::RuntimeAssetRegistry& registry) {
  WorldAssetSet assets = resolveWorldAssets([&registry](std::string_view assetId) {
    const hh::renderer::AssetHandle handle = registry.resolve(assetId);
    return visualFromRuntimeAsset(handle, registry.asset(handle));
  });

  assets.catalog.reserve(registry.size());
  for (const auto& asset : registry.assets()) {
    const auto handle = registry.resolve(asset.assetId);
    assets.catalog.emplace(
        asset.assetId, visualFromRuntimeAsset(handle, asset));
  }
  return assets;
}

WorldAssetSet loadWorldAssetsFromDirectory(
    hh::renderer::RuntimeAssetRegistry& registry,
    const std::filesystem::path& cookedRoot,
    RuntimeWorldAssetLoadMode mode) {
  if (mode == RuntimeWorldAssetLoadMode::FullMilestone) {
    registry.loadDirectoryAssetRange(
        cookedRoot, "HH_A", 1u, 700u, requiredWorldAssetIds());
    return worldAssetsFromRegistry(registry);
  }

  const auto legacy = requiredWorldAssetIds();
  const auto v2 = runtimeWorldPresentationAssetIds();
  std::vector<std::string_view> shippingIds;
  shippingIds.reserve(legacy.size() + v2.size());
  shippingIds.insert(shippingIds.end(), legacy.begin(), legacy.end());
  shippingIds.insert(shippingIds.end(), v2.begin(), v2.end());
  registry.loadDirectorySubset(cookedRoot, shippingIds);
  return worldAssetsFromRegistry(registry);
}

} // namespace hh::client
