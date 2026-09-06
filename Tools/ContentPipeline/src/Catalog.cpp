#include "hh/assets/Catalog.h"
#include <stdexcept>

namespace hh::assets {
namespace {
bool has_sidecar_suffix(const std::filesystem::path& path) {
    const auto name = path.filename().string();
    constexpr std::string_view suffix = ".asset.json";
    return name.size() >= suffix.size() && name.compare(name.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::filesystem::path export_from_sidecar(const std::filesystem::path& sidecar) {
    auto name = sidecar.filename().string();
    constexpr std::string_view suffix = ".asset.json";
    name.resize(name.size() - suffix.size());
    return sidecar.parent_path() / name;
}
}

AssetCatalog AssetCatalog::scan(const std::filesystem::path& exports_root) {
    AssetCatalog catalog;
    catalog.exports_root_ = std::filesystem::absolute(exports_root).lexically_normal();
    if (!std::filesystem::exists(catalog.exports_root_)) {
        throw std::runtime_error("exports root does not exist: " + catalog.exports_root_.string());
    }
    auto art_root = catalog.exports_root_.parent_path();
    catalog.repository_root_ = art_root.parent_path();

    for (const auto& entry : std::filesystem::recursive_directory_iterator(catalog.exports_root_)) {
        if (!entry.is_regular_file() || !has_sidecar_suffix(entry.path())) continue;
        AssetRecord record;
        record.sidecar_path = std::filesystem::absolute(entry.path()).lexically_normal();
        record.export_path = export_from_sidecar(record.sidecar_path).lexically_normal();
        record.metadata = load_metadata(record.sidecar_path);
        const auto source = std::filesystem::path(record.metadata.source);
        record.source_path = source.is_absolute() ? source.lexically_normal() : (catalog.repository_root_ / source).lexically_normal();
        const auto [it, inserted] = catalog.records_.emplace(record.metadata.asset_id, std::move(record));
        if (!inserted) {
            throw std::runtime_error("duplicate asset_id: " + it->first);
        }
    }
    return catalog;
}

const AssetRecord& AssetCatalog::by_id(std::string_view id) const {
    const auto it = records_.find(id);
    if (it == records_.end()) throw std::out_of_range("unknown asset_id: " + std::string(id));
    return it->second;
}

const AssetRecord& AssetCatalog::resolve(std::string_view id_or_path) const {
    if (const auto it = records_.find(id_or_path); it != records_.end()) return it->second;
    const auto candidate = std::filesystem::absolute(std::filesystem::path(id_or_path)).lexically_normal();
    for (const auto& [id, record] : records_) {
        static_cast<void>(id);
        if (record.sidecar_path == candidate || record.export_path == candidate || record.source_path == candidate) return record;
    }
    throw std::out_of_range("cannot resolve asset: " + std::string(id_or_path));
}
}
