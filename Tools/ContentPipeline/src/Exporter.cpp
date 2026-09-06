#include "hh/assets/Exporter.h"
#include <cstdlib>

namespace hh::assets {
ExportResult export_asset(const AssetRecord& record, const std::filesystem::path& repository_root) {
    static_cast<void>(repository_root);
    const char* configured = std::getenv("HOTEL_HAVEN_BLENDER");
    if (configured == nullptr || *configured == '\0') {
        return {false, 2, "HOTEL_HAVEN_BLENDER is not configured"};
    }
    const std::filesystem::path blender(configured);
    if (!std::filesystem::exists(blender)) {
        return {false, 2, "HOTEL_HAVEN_BLENDER does not exist: " + blender.string()};
    }
    if (!std::filesystem::exists(record.source_path)) {
        return {false, 1, "asset source does not exist: " + record.source_path.string()};
    }
    if (record.source_path.extension() != ".blend") {
        return {false, 3, "HMG-070 export orchestration currently accepts Blender .blend sources only"};
    }
    return {false, 3, "HMG-070 does not define type-specific Blender export switches; install the HMG-071 through HMG-079 exporter adapter before invoking export"};
}
}
