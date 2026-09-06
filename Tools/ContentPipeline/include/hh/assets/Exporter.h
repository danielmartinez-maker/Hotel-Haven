#pragma once
#include "hh/assets/Catalog.h"
#include <filesystem>
#include <string>

namespace hh::assets {
struct ExportResult {
    bool success{};
    int exit_code{};
    std::string message;
};
ExportResult export_asset(const AssetRecord& record, const std::filesystem::path& repository_root);
}
