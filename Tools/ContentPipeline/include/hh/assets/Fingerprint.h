#pragma once
#include "hh/assets/Catalog.h"
#include "hh/assets/DependencyGraph.h"
#include <string>

namespace hh::assets {
struct FingerprintSettings {
    std::string importer_version;
    std::string cooker_version;
    std::string compression_settings;
    std::string platform_target;
};

std::string compute_fingerprint(
    const AssetRecord& record,
    const AssetCatalog& catalog,
    const DependencyGraph& graph,
    const FingerprintSettings& settings);
}
