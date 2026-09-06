#pragma once
#include <stdexcept>
#include <string>
#include <string_view>

namespace hh::assets {

enum class AssetType {
    StaticMesh,
    SkinnedMesh,
    Skeleton,
    AnimationClip,
    AnimationSet,
    Material,
    Texture,
    Prefab,
    UIAtlas,
    Font,
    VFX,
    AudioClip,
    AudioBank
};

enum class Severity { Blocker, Critical, Major, Minor };

enum class LifecycleState {
    Requested,
    Concept,
    Blockout,
    Production,
    TechArt,
    Review,
    Approved,
    Cooked,
    InGameVerified,
    ReleaseReady
};

enum class CutawayPolicy { Normal, FadeWhenForeground, HideUpperSection, NeverCut };

struct Diagnostic {
    Severity severity{};
    std::string code;
    std::string message;
};

std::string_view to_string(AssetType type) noexcept;
AssetType asset_type_from_string(std::string_view text);
std::string_view to_string(LifecycleState state) noexcept;
LifecycleState lifecycle_state_from_string(std::string_view text);
std::string_view to_string(CutawayPolicy policy) noexcept;
CutawayPolicy cutaway_policy_from_string(std::string_view text);
bool is_release_blocking(Severity severity) noexcept;
bool can_transition(LifecycleState from, LifecycleState to, bool review_rejected = false) noexcept;

}
