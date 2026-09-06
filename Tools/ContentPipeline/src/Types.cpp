#include "hh/assets/Types.h"
#include <array>
#include <utility>

namespace hh::assets {
namespace {
constexpr std::array<std::pair<AssetType, std::string_view>, 13> kAssetTypes{{
    {AssetType::StaticMesh, "StaticMeshAsset"},
    {AssetType::SkinnedMesh, "SkinnedMeshAsset"},
    {AssetType::Skeleton, "SkeletonAsset"},
    {AssetType::AnimationClip, "AnimationClipAsset"},
    {AssetType::AnimationSet, "AnimationSetAsset"},
    {AssetType::Material, "MaterialAsset"},
    {AssetType::Texture, "TextureAsset"},
    {AssetType::Prefab, "PrefabAsset"},
    {AssetType::UIAtlas, "UIAtlasAsset"},
    {AssetType::Font, "FontAsset"},
    {AssetType::VFX, "VFXAsset"},
    {AssetType::AudioClip, "AudioClipAsset"},
    {AssetType::AudioBank, "AudioBankAsset"}
}};
constexpr std::array<std::pair<LifecycleState, std::string_view>, 10> kLifecycle{{
    {LifecycleState::Requested, "REQUESTED"},
    {LifecycleState::Concept, "CONCEPT"},
    {LifecycleState::Blockout, "BLOCKOUT"},
    {LifecycleState::Production, "PRODUCTION"},
    {LifecycleState::TechArt, "TECH_ART"},
    {LifecycleState::Review, "REVIEW"},
    {LifecycleState::Approved, "APPROVED"},
    {LifecycleState::Cooked, "COOKED"},
    {LifecycleState::InGameVerified, "IN_GAME_VERIFIED"},
    {LifecycleState::ReleaseReady, "RELEASE_READY"}
}};
constexpr std::array<std::pair<CutawayPolicy, std::string_view>, 4> kCutaway{{
    {CutawayPolicy::Normal, "normal"},
    {CutawayPolicy::FadeWhenForeground, "fade_when_foreground"},
    {CutawayPolicy::HideUpperSection, "hide_upper_section"},
    {CutawayPolicy::NeverCut, "never_cut"}
}};
}

std::string_view to_string(AssetType type) noexcept {
    for (const auto& [value, name] : kAssetTypes) if (value == type) return name;
    return "UnknownAsset";
}

AssetType asset_type_from_string(std::string_view text) {
    for (const auto& [value, name] : kAssetTypes) if (name == text) return value;
    throw std::invalid_argument("unsupported asset type: " + std::string(text));
}

std::string_view to_string(LifecycleState state) noexcept {
    for (const auto& [value, name] : kLifecycle) if (value == state) return name;
    return "UNKNOWN";
}

LifecycleState lifecycle_state_from_string(std::string_view text) {
    for (const auto& [value, name] : kLifecycle) if (name == text) return value;
    throw std::invalid_argument("unsupported lifecycle state: " + std::string(text));
}

std::string_view to_string(CutawayPolicy policy) noexcept {
    for (const auto& [value, name] : kCutaway) if (value == policy) return name;
    return "normal";
}

CutawayPolicy cutaway_policy_from_string(std::string_view text) {
    for (const auto& [value, name] : kCutaway) if (name == text) return value;
    throw std::invalid_argument("unsupported cutaway policy: " + std::string(text));
}

bool is_release_blocking(Severity severity) noexcept {
    return severity != Severity::Minor;
}

bool can_transition(LifecycleState from, LifecycleState to, bool review_rejected) noexcept {
    if (from == LifecycleState::Review && review_rejected) {
        return to == LifecycleState::Concept || to == LifecycleState::Blockout ||
               to == LifecycleState::Production || to == LifecycleState::TechArt;
    }
    const auto current = static_cast<int>(from);
    const auto next = static_cast<int>(to);
    return next == current + 1;
}
}
