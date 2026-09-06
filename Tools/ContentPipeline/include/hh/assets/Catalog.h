#pragma once
#include "hh/assets/Metadata.h"
#include <filesystem>
#include <map>
#include <string>
#include <string_view>

namespace hh::assets {
struct AssetRecord {
    AssetMetadata metadata;
    std::filesystem::path sidecar_path;
    std::filesystem::path export_path;
    std::filesystem::path source_path;
};

class AssetCatalog {
public:
    static AssetCatalog scan(const std::filesystem::path& exports_root);
    const AssetRecord& by_id(std::string_view id) const;
    const AssetRecord& resolve(std::string_view id_or_path) const;
    std::size_t size() const noexcept { return records_.size(); }
    const std::map<std::string, AssetRecord, std::less<>>& records() const noexcept { return records_; }
    const std::filesystem::path& repository_root() const noexcept { return repository_root_; }
    const std::filesystem::path& exports_root() const noexcept { return exports_root_; }

private:
    std::filesystem::path repository_root_;
    std::filesystem::path exports_root_;
    std::map<std::string, AssetRecord, std::less<>> records_;
};
}
