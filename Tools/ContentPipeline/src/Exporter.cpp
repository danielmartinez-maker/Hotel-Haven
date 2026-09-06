#include "hh/assets/Exporter.h"
#include <cstdlib>
#include <string>

namespace hh::assets {
namespace {
std::string blender_from_environment() {
#ifdef _WIN32
    char* value = nullptr;
    std::size_t length = 0;
    if (_dupenv_s(&value, &length, "HOTEL_HAVEN_BLENDER") != 0) {
        return {};
    }
    std::string result = value == nullptr ? std::string{} : std::string(value);
    std::free(value);
    return result;
#else
    const char* value = std::getenv("HOTEL_HAVEN_BLENDER");
    return value == nullptr ? std::string{} : std::string(value);
#endif
}
}

ExportResult export_asset(const AssetRecord& record, const std::filesystem::path& repository_root) {
    static_cast<void>(repository_root);
    const std::string configured = blender_from_environment();
    if (configured.empty()) {
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
