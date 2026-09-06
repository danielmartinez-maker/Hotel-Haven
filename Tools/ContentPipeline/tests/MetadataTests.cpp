#include "Test.h"
#include "hh/assets/Json.h"
#include "hh/assets/Metadata.h"
#include "hh/assets/Types.h"
#include <array>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

using namespace hh::assets;
namespace fs = std::filesystem;

namespace {
fs::path temp_dir() {
    static int serial = 0;
    auto p = fs::temp_directory_path() / ("hh_asset_metadata_" + std::to_string(++serial));
    fs::remove_all(p);
    fs::create_directories(p);
    return p;
}
void write_text(const fs::path& p, const std::string& text) {
    std::ofstream out(p, std::ios::binary);
    out << text;
}
std::string valid_sidecar(std::string_view asset_type = "StaticMeshAsset", std::string_view units = "meters") {
    return std::string("{\n") +
        "  \"schema\":1,\n" +
        "  \"asset_id\":\"asset.prop.guestroom.bed.king.modern_01\",\n" +
        "  \"asset_type\":\"" + std::string(asset_type) + "\",\n" +
        "  \"source\":\"Art/Source/Props/Guestroom/Beds/Modern01.blend\",\n" +
        "  \"units\":\"" + std::string(units) + "\",\n" +
        "  \"lod_policy\":\"prop_standard\",\n" +
        "  \"collision_policy\":\"simple_authored\",\n" +
        "  \"material_slots\":[\"frame\",\"linen\",\"metal\"],\n" +
        "  \"tags\":[\"guestroom\",\"bed\",\"king\"],\n" +
        "  \"dependencies\":[],\n" +
        "  \"cutaway_policy\":\"fade_when_foreground\",\n" +
        "  \"source_revision\":7,\n" +
        "  \"metadata_revision\":4,\n" +
        "  \"cooker_schema\":3,\n" +
        "  \"lifecycle_state\":\"APPROVED\",\n" +
        "  \"content_owner\":\"art\",\n" +
        "  \"technical_reviewer\":\"tech\",\n" +
        "  \"art_reviewer\":\"lead\",\n" +
        "  \"dependent_feature_owner\":\"hotel-room\",\n" +
        "  \"milestone\":\"vertical-slice\"\n" +
        "}";
}
}

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
HH_TEST("json parser handles escapes arrays numbers booleans and null") {
    const auto value = parse_json(R"({"s":"A\n\u0042","a":[1,true,false,null]})");
    HH_REQUIRE(value.is_object());
    HH_REQUIRE(value.at("s").as_string() == "A\nB");
    HH_REQUIRE(value.at("a").as_array().size() == 4);
    HH_REQUIRE(value.at("a").as_array()[0].as_number() == 1.0);
    HH_REQUIRE(value.at("a").as_array()[1].as_bool());
    HH_REQUIRE(value.at("a").as_array()[3].is_null());
}
HH_TEST("json parser rejects malformed trailing data") {
    bool threw = false;
    try { static_cast<void>(parse_json("{} trailing")); } catch (const std::exception&) { threw = true; }
    HH_REQUIRE(threw);
}
HH_TEST("valid sidecar loads exact HMG-070 fields") {
    const auto dir = temp_dir();
    const auto path = dir / "bed.glb.asset.json";
    write_text(path, valid_sidecar());
    const auto metadata = load_metadata(path);
    HH_REQUIRE(metadata.schema == 1);
    HH_REQUIRE(metadata.asset_id == "asset.prop.guestroom.bed.king.modern_01");
    HH_REQUIRE(metadata.asset_type == AssetType::StaticMesh);
    HH_REQUIRE(metadata.material_slots.size() == 3);
    HH_REQUIRE(metadata.cutaway_policy.has_value());
    HH_REQUIRE(*metadata.cutaway_policy == CutawayPolicy::FadeWhenForeground);
    HH_REQUIRE(metadata.lifecycle_state == LifecycleState::Approved);
    HH_REQUIRE(validate_metadata(metadata).empty());
}
HH_TEST("missing each required sidecar field fails import") {
    const std::array<std::string_view, 10> required = {
        "schema","asset_id","asset_type","source","units","lod_policy","collision_policy","material_slots","tags","dependencies"};
    for (const auto field : required) {
        auto text = valid_sidecar();
        const auto needle = std::string("  \"") + std::string(field) + "\"";
        const auto begin = text.find(needle);
        HH_REQUIRE(begin != std::string::npos);
        auto end = text.find('\n', begin);
        if (end == std::string::npos) end = text.size(); else ++end;
        text.erase(begin, end - begin);
        const auto dir = temp_dir();
        const auto path = dir / "missing.asset.json";
        write_text(path, text);
        bool threw = false;
        try { static_cast<void>(load_metadata(path)); } catch (const std::exception&) { threw = true; }
        HH_REQUIRE(threw);
    }
}
HH_TEST("unsupported schema is rejected") {
    auto text = valid_sidecar();
    const auto pos = text.find("\"schema\":1");
    text.replace(pos, std::string("\"schema\":1").size(), "\"schema\":2");
    const auto dir = temp_dir(); const auto path = dir / "x.asset.json"; write_text(path, text);
    const auto metadata = load_metadata(path);
    HH_REQUIRE(!validate_metadata(metadata).empty());
}
HH_TEST("invalid asset type fails import") {
    auto text = valid_sidecar();
    const auto pos = text.find("StaticMeshAsset"); text.replace(pos, 15, "MadeUpAssetType");
    const auto dir = temp_dir(); const auto path = dir / "x.asset.json"; write_text(path, text);
    bool threw = false; try { static_cast<void>(load_metadata(path)); } catch (const std::exception&) { threw = true; }
    HH_REQUIRE(threw);
}
HH_TEST("3D assets require meters but textures may use pixels") {
    const auto dir = temp_dir();
    const auto mesh = dir / "mesh.asset.json"; write_text(mesh, valid_sidecar("StaticMeshAsset", "centimeters"));
    HH_REQUIRE(!validate_metadata(load_metadata(mesh)).empty());
    const auto texture = dir / "texture.asset.json"; write_text(texture, valid_sidecar("TextureAsset", "pixels"));
    HH_REQUIRE(validate_metadata(load_metadata(texture)).empty());
}
HH_TEST("invalid cutaway policy fails import") {
    auto text = valid_sidecar();
    const auto pos = text.find("fade_when_foreground"); text.replace(pos, 20, "sometimes_hide_front");
    const auto dir = temp_dir(); const auto path = dir / "x.asset.json"; write_text(path, text);
    bool threw = false; try { static_cast<void>(load_metadata(path)); } catch (const std::exception&) { threw = true; }
    HH_REQUIRE(threw);
}
HH_TEST("canonical metadata output is stable and key ordered") {
    const auto dir = temp_dir(); const auto path = dir / "bed.asset.json"; write_text(path, valid_sidecar());
    const auto metadata = load_metadata(path);
    const auto a = canonicalize_metadata(metadata);
    const auto b = canonicalize_metadata(metadata);
    HH_REQUIRE(a == b);
    HH_REQUIRE(a.find("\"asset_id\"") < a.find("\"asset_type\""));
    HH_REQUIRE(a.find("\"schema\"") > a.find("\"material_slots\""));
}
