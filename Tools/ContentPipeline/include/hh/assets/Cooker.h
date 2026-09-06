#pragma once
#include "hh/assets/Catalog.h"
#include "hh/assets/DependencyGraph.h"
#include "hh/assets/Fingerprint.h"
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace hh::assets {
struct CookOptions {
    std::filesystem::path repository_root;
    std::filesystem::path cooked_root;
    FingerprintSettings fingerprint;
};
struct CookResult {
    std::string asset_id;
    bool cooked{};
    std::filesystem::path output;
    std::string fingerprint;
};

CookResult cook_one(const AssetCatalog& catalog, const DependencyGraph& graph, std::string_view asset_id, const CookOptions& options);
std::vector<CookResult> cook_all(const AssetCatalog& catalog, const DependencyGraph& graph, const CookOptions& options);
std::vector<CookResult> cook_changed(const AssetCatalog& catalog, const DependencyGraph& graph, const CookOptions& options);
}
