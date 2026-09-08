#include "hh/assets/Catalog.h"
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

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

std::filesystem::path infer_repository_root(const std::filesystem::path& exports_root) {
    auto current = exports_root;
    for (;;) {
        if (current.filename() == "Exports" && current.parent_path().filename() == "Art") {
            return current.parent_path().parent_path().lexically_normal();
        }
        const auto parent = current.parent_path();
        if (parent.empty() || parent == current) break;
        current = parent;
    }

    // Preserve compatibility for callers that provide a conventional top-level
    // exports directory without the Hotel Haven Art/Exports path names.
    return exports_root.parent_path().parent_path().lexically_normal();
}

bool has_windows_absolute_prefix(std::string_view value) {
    if (value.size() >= 3 && std::isalpha(static_cast<unsigned char>(value[0])) != 0 &&
        value[1] == ':' && (value[2] == '/' || value[2] == '\\')) {
        return true;
    }
    return value.size() >= 2 &&
           ((value[0] == '/' && value[1] == '/') || (value[0] == '\\' && value[1] == '\\'));
}

bool contains_parent_component(std::string_view value) {
    std::size_t begin = 0;
    for (std::size_t i = 0; i <= value.size(); ++i) {
        if (i != value.size() && value[i] != '/' && value[i] != '\\') continue;
        if (value.substr(begin, i - begin) == "..") return true;
        begin = i + 1;
    }
    return false;
}

bool path_within_root(
    const std::filesystem::path& candidate,
    const std::filesystem::path& repository_root) {
    const auto relative = candidate.lexically_relative(repository_root);
    return !relative.empty() && *relative.begin() != "..";
}

std::filesystem::path resolve_repository_source(
    std::string_view source_text,
    const std::filesystem::path& repository_root) {
    const auto source = std::filesystem::path(source_text);
    if (source_text.empty() || source.is_absolute() || has_windows_absolute_prefix(source_text) ||
        contains_parent_component(source_text)) {
        throw std::runtime_error("asset source must be repository-relative without parent traversal: " + std::string(source_text));
    }

    const auto candidate = (repository_root / source).lexically_normal();
    if (!path_within_root(candidate, repository_root)) {
        throw std::runtime_error("asset source escapes repository root: " + candidate.string());
    }

    // Lexical containment is insufficient when an in-repository component is a
    // symlink. Resolve the existing path prefix and reject any target that leaves
    // the repository before fingerprinting or cooking can read it.
    const auto canonical_root = std::filesystem::weakly_canonical(repository_root);
    const auto canonical_candidate = std::filesystem::weakly_canonical(candidate);
    if (!path_within_root(canonical_candidate, canonical_root)) {
        throw std::runtime_error("asset source resolves outside repository root: " + candidate.string());
    }
    return candidate;
}
}

AssetCatalog AssetCatalog::scan(const std::filesystem::path& exports_root) {
    AssetCatalog catalog;
    catalog.exports_root_ = std::filesystem::absolute(exports_root).lexically_normal();
    if (!std::filesystem::exists(catalog.exports_root_)) {
        throw std::runtime_error("exports root does not exist: " + catalog.exports_root_.string());
    }
    catalog.repository_root_ = infer_repository_root(catalog.exports_root_);

    for (const auto& entry : std::filesystem::recursive_directory_iterator(catalog.exports_root_)) {
        if (!entry.is_regular_file() || !has_sidecar_suffix(entry.path())) continue;
        AssetRecord record;
        record.sidecar_path = std::filesystem::absolute(entry.path()).lexically_normal();
        record.export_path = export_from_sidecar(record.sidecar_path).lexically_normal();
        record.metadata = load_metadata(record.sidecar_path);
        record.source_path = resolve_repository_source(record.metadata.source, catalog.repository_root_);
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
