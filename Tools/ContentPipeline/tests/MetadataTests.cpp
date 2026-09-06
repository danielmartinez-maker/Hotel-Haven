#include "Test.h"
#include "hh/assets/Types.h"
#include <array>
#include <string_view>
using namespace hh::assets;
HH_TEST("asset types round trip") {
    constexpr std::array<std::string_view, 13> names = {
        "StaticMeshAsset","SkinnedMeshAsset","SkeletonAsset","AnimationClipAsset","AnimationSetAsset",
        "MaterialAsset","TextureAsset","PrefabAsset","UIAtlasAsset","FontAsset","VFXAsset","AudioClipAsset","AudioBankAsset"};
    for (auto name : names) HH_REQUIRE(to_string(asset_type_from_string(name)) == name);
}
HH_TEST("lifecycle only allows sequential forward transitions") {
    HH_REQUIRE(can_transition(LifecycleState::Requested, LifecycleState::Concept));
    HH_REQUIRE(can_transition(LifecycleState::Review, LifecycleState::Approved));
    HH_REQUIRE(!can_transition(LifecycleState::Review, LifecycleState::ReleaseReady));
}
HH_TEST("review rejection can return to technical art") {
    HH_REQUIRE(!can_transition(LifecycleState::Review, LifecycleState::TechArt));
    HH_REQUIRE(can_transition(LifecycleState::Review, LifecycleState::TechArt, true));
}
HH_TEST("release blocking severities are exact") {
    HH_REQUIRE(is_release_blocking(Severity::Blocker));
    HH_REQUIRE(is_release_blocking(Severity::Critical));
    HH_REQUIRE(is_release_blocking(Severity::Major));
    HH_REQUIRE(!is_release_blocking(Severity::Minor));
}
