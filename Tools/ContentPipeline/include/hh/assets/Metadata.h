#pragma once
#include "hh/assets/Types.h"
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace hh::assets {
struct AssetMetadata {
    int schema{};
    std::string asset_id;
    AssetType asset_type{};
    std::string source;
    std::string units;
    std::string lod_policy;
    std::string collision_policy;
    std::vector<std::string> material_slots;
    std::vector<std::string> tags;
    std::vector<std::string> dependencies;
    std::optional<CutawayPolicy> cutaway_policy;
    std::optional<std::string> pivot_exception_reason;
    std::optional<std::int64_t> source_revision;
    std::optional<std::int64_t> metadata_revision;
    std::optional<std::int64_t> cooker_schema;
    std::optional<LifecycleState> lifecycle_state;
    std::optional<std::string> content_owner;
    std::optional<std::string> technical_reviewer;
    std::optional<std::string> art_reviewer;
    std::optional<std::string> dependent_feature_owner;
    std::optional<std::string> milestone;
};

AssetMetadata load_metadata(const std::filesystem::path& sidecar);
std::vector<Diagnostic> validate_metadata(const AssetMetadata& metadata);
std::string canonicalize_metadata(const AssetMetadata& metadata);
bool is_3d_asset_type(AssetType type) noexcept;
}
