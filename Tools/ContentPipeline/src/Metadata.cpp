#include "hh/assets/Metadata.h"
#include "hh/assets/Json.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace hh::assets {
namespace {
std::string read_text(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("cannot open metadata: " + path.string());
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

const JsonValue& required(const JsonValue& root, std::string_view key) {
    const auto* value = root.find(key);
    if (value == nullptr) throw std::runtime_error("missing required metadata field: " + std::string(key));
    return *value;
}

std::string required_string(const JsonValue& root, std::string_view key) { return required(root, key).as_string(); }

std::vector<std::string> required_string_array(const JsonValue& root, std::string_view key) {
    std::vector<std::string> result;
    for (const auto& value : required(root, key).as_array()) result.push_back(value.as_string());
    return result;
}

std::int64_t integer_value(const JsonValue& value, std::string_view field) {
    const double number = value.as_number();
    if (std::floor(number) != number) throw std::runtime_error("metadata field must be an integer: " + std::string(field));
    return static_cast<std::int64_t>(number);
}

std::optional<std::string> optional_string(const JsonValue& root, std::string_view key) {
    const auto* value = root.find(key);
    if (value == nullptr) return std::nullopt;
    return value->as_string();
}

std::optional<std::int64_t> optional_integer(const JsonValue& root, std::string_view key) {
    const auto* value = root.find(key);
    if (value == nullptr) return std::nullopt;
    return integer_value(*value, key);
}

std::string escape_json(std::string_view text) {
    std::string out;
    out.push_back('"');
    constexpr char hex[] = "0123456789abcdef";
    for (const unsigned char c : text) {
        switch (c) {
        case '"': out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\b': out += "\\b"; break;
        case '\f': out += "\\f"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:
            if (c < 0x20u) {
                out += "\\u00";
                out.push_back(hex[(c >> 4u) & 0x0Fu]);
                out.push_back(hex[c & 0x0Fu]);
            } else {
                out.push_back(static_cast<char>(c));
            }
        }
    }
    out.push_back('"');
    return out;
}

std::string string_array_json(const std::vector<std::string>& values) {
    std::string out = "[";
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i != 0) out.push_back(',');
        out += escape_json(values[i]);
    }
    out.push_back(']');
    return out;
}
}

bool is_3d_asset_type(AssetType type) noexcept {
    switch (type) {
    case AssetType::StaticMesh:
    case AssetType::SkinnedMesh:
    case AssetType::Skeleton:
    case AssetType::AnimationClip:
    case AssetType::AnimationSet:
    case AssetType::Prefab:
    case AssetType::VFX:
        return true;
    case AssetType::Material:
    case AssetType::Texture:
    case AssetType::UIAtlas:
    case AssetType::Font:
    case AssetType::AudioClip:
    case AssetType::AudioBank:
        return false;
    }
    return false;
}

AssetMetadata load_metadata(const std::filesystem::path& sidecar) {
    const auto root = parse_json(read_text(sidecar));
    if (!root.is_object()) throw std::runtime_error("metadata root must be an object");
    AssetMetadata metadata;
    metadata.schema = static_cast<int>(integer_value(required(root, "schema"), "schema"));
    metadata.asset_id = required_string(root, "asset_id");
    metadata.asset_type = asset_type_from_string(required_string(root, "asset_type"));
    metadata.source = required_string(root, "source");
    metadata.units = required_string(root, "units");
    metadata.lod_policy = required_string(root, "lod_policy");
    metadata.collision_policy = required_string(root, "collision_policy");
    metadata.material_slots = required_string_array(root, "material_slots");
    metadata.tags = required_string_array(root, "tags");
    metadata.dependencies = required_string_array(root, "dependencies");

    if (const auto value = optional_string(root, "cutaway_policy")) metadata.cutaway_policy = cutaway_policy_from_string(*value);
    metadata.pivot_exception_reason = optional_string(root, "pivot_exception_reason");
    metadata.source_revision = optional_integer(root, "source_revision");
    metadata.metadata_revision = optional_integer(root, "metadata_revision");
    metadata.cooker_schema = optional_integer(root, "cooker_schema");
    if (const auto value = optional_string(root, "lifecycle_state")) metadata.lifecycle_state = lifecycle_state_from_string(*value);
    metadata.content_owner = optional_string(root, "content_owner");
    metadata.technical_reviewer = optional_string(root, "technical_reviewer");
    metadata.art_reviewer = optional_string(root, "art_reviewer");
    metadata.dependent_feature_owner = optional_string(root, "dependent_feature_owner");
    metadata.milestone = optional_string(root, "milestone");
    return metadata;
}

std::vector<Diagnostic> validate_metadata(const AssetMetadata& metadata) {
    std::vector<Diagnostic> diagnostics;
    if (metadata.schema != 1) diagnostics.push_back({Severity::Blocker, "metadata.schema.unsupported", "metadata schema must be 1"});
    if (metadata.asset_id.empty()) diagnostics.push_back({Severity::Blocker, "metadata.asset_id.empty", "asset_id must not be empty"});
    if (metadata.source.empty()) diagnostics.push_back({Severity::Blocker, "metadata.source.empty", "source must not be empty"});
    if (metadata.lod_policy.empty()) diagnostics.push_back({Severity::Blocker, "metadata.lod_policy.empty", "lod_policy must not be empty"});
    if (metadata.collision_policy.empty()) diagnostics.push_back({Severity::Blocker, "metadata.collision_policy.empty", "collision_policy must not be empty"});
    if (is_3d_asset_type(metadata.asset_type) && metadata.units != "meters") {
        diagnostics.push_back({Severity::Critical, "metadata.units.3d", "3D assets must use meters"});
    }
    return diagnostics;
}

std::string canonicalize_metadata(const AssetMetadata& m) {
    std::vector<std::pair<std::string, std::string>> fields;
    auto add_string = [&](std::string key, const std::string& value) { fields.emplace_back(std::move(key), escape_json(value)); };
    auto add_integer = [&](std::string key, std::int64_t value) { fields.emplace_back(std::move(key), std::to_string(value)); };
    add_string("asset_id", m.asset_id);
    add_string("asset_type", std::string(to_string(m.asset_type)));
    if (m.art_reviewer) add_string("art_reviewer", *m.art_reviewer);
    add_string("collision_policy", m.collision_policy);
    if (m.content_owner) add_string("content_owner", *m.content_owner);
    if (m.cooker_schema) add_integer("cooker_schema", *m.cooker_schema);
    if (m.cutaway_policy) add_string("cutaway_policy", std::string(to_string(*m.cutaway_policy)));
    fields.emplace_back("dependencies", string_array_json(m.dependencies));
    if (m.dependent_feature_owner) add_string("dependent_feature_owner", *m.dependent_feature_owner);
    if (m.lifecycle_state) add_string("lifecycle_state", std::string(to_string(*m.lifecycle_state)));
    add_string("lod_policy", m.lod_policy);
    fields.emplace_back("material_slots", string_array_json(m.material_slots));
    if (m.metadata_revision) add_integer("metadata_revision", *m.metadata_revision);
    if (m.milestone) add_string("milestone", *m.milestone);
    if (m.pivot_exception_reason) add_string("pivot_exception_reason", *m.pivot_exception_reason);
    add_integer("schema", m.schema);
    add_string("source", m.source);
    if (m.source_revision) add_integer("source_revision", *m.source_revision);
    fields.emplace_back("tags", string_array_json(m.tags));
    if (m.technical_reviewer) add_string("technical_reviewer", *m.technical_reviewer);
    add_string("units", m.units);
    std::sort(fields.begin(), fields.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    std::ostringstream out;
    out << '{';
    for (std::size_t i = 0; i < fields.size(); ++i) {
        if (i != 0) out << ',';
        out << escape_json(fields[i].first) << ':' << fields[i].second;
    }
    out << '}';
    return out.str();
}
}
