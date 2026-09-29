#include "hh/assets/Catalog.h"
#include <algorithm>
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

bool is_shadowed_legacy_sidecar(const std::filesystem::path& sidecar) {
    const auto export_path = export_from_sidecar(sidecar);
    if (std::filesystem::is_regular_file(export_path)) return false;
    auto base = export_path.filename().string();
    const auto is_known_json_asset = [&](std::string_view suffix) {
        return base.size() >= suffix.size() &&
               base.compare(base.size() - suffix.size(), suffix.size(), suffix) == 0;
    };
    const auto extension = (is_known_json_asset(".anim") || is_known_json_asset(".animset") ||
                            is_known_json_asset(".skeleton")) ? ".json" : ".glb";
    const auto canonical_export = sidecar.parent_path() / (base + extension);
    const auto canonical_sidecar = std::filesystem::path(canonical_export.string() + ".asset.json");
    if (!std::filesystem::is_regular_file(canonical_export) ||
        !std::filesystem::is_regular_file(canonical_sidecar)) return false;

    const auto alias_metadata = load_metadata(sidecar);
    const auto canonical_metadata = load_metadata(canonical_sidecar);
    if (alias_metadata.asset_id != canonical_metadata.asset_id ||
        alias_metadata.source != canonical_metadata.source ||
        !std::all_of(alias_metadata.dependencies.begin(), alias_metadata.dependencies.end(),
                     [&](const auto& dependency) {
                         return std::find(canonical_metadata.dependencies.begin(),
                                          canonical_metadata.dependencies.end(), dependency) !=
                                canonical_metadata.dependencies.end();
                     })) {
        throw std::runtime_error("conflicting legacy sidecar alias: " + sidecar.string());
    }
    return true;
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

    for (auto entry = std::filesystem::recursive_directory_iterator(catalog.exports_root_);
         entry != std::filesystem::recursive_directory_iterator(); ++entry) {
        if (entry->is_directory() && entry->path().filename() == ".rsync-tmp") {
            entry.disable_recursion_pending();
            continue;
        }
        if (!entry->is_regular_file() || !has_sidecar_suffix(entry->path())) continue;
        if (is_shadowed_legacy_sidecar(entry->path())) continue;
        AssetRecord record;
        record.sidecar_path = std::filesystem::absolute(entry->path()).lexically_normal();
        record.export_path = export_from_sidecar(record.sidecar_path).lexically_normal();
        record.metadata = load_metadata(record.sidecar_path);
        if (!std::filesystem::is_regular_file(record.export_path)) {
            throw std::runtime_error("missing export paired with " + record.sidecar_path.string());
        }
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
