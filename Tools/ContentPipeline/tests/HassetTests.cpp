#include "Test.h"
#include "hh/assets/Hasset.h"
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

using namespace hh::assets;
namespace {
HassetDocument sample() {
    HassetDocument d;
    d.type = AssetType::StaticMesh;
    d.asset_id = "asset.prop.bed";
    d.fingerprint = "0123456789abcdef";
    d.dependencies = {"asset.z", "asset.a"};
    d.source_path = "Art/Source/Props/Bed.blend";
    d.sidecar_path = "Art/Exports/Props/Bed.glb.asset.json";
    d.units = "meters";
    d.lod_policy = "prop_standard";
    d.collision_policy = "simple_authored";
    d.cutaway_policy = "fade_when_foreground";
    d.pivot_profile = "floor_contact_center";
    d.interaction_anchors = {"INT_USE_01", "INT_REPAIR_01", "INT_USE_01"};
    d.payload = {std::byte{1}, std::byte{2}, std::byte{3}, std::byte{255}};
    return d;
}
bool parse_throws(const std::vector<std::byte>& bytes) {
    try { static_cast<void>(parse_hasset(bytes)); return false; } catch (const std::exception&) { return true; }
}
void append_u32(std::vector<std::byte>& out, std::uint32_t value) {
    for (unsigned shift = 0; shift < 32u; shift += 8u) out.push_back(static_cast<std::byte>((value >> shift) & 0xffu));
}
void append_u64(std::vector<std::byte>& out, std::uint64_t value) {
    for (unsigned shift = 0; shift < 64u; shift += 8u) out.push_back(static_cast<std::byte>((value >> shift) & 0xffu));
}
void append_string(std::vector<std::byte>& out, std::string_view value) {
    HH_REQUIRE(value.size() <= std::numeric_limits<std::uint32_t>::max());
    append_u32(out, static_cast<std::uint32_t>(value.size()));
    for (const unsigned char c : value) out.push_back(static_cast<std::byte>(c));
}
std::vector<std::byte> legacy_v1_fixture() {
    std::vector<std::byte> out = {
        std::byte{'H'}, std::byte{'H'}, std::byte{'A'}, std::byte{'S'},
        std::byte{'S'}, std::byte{'E'}, std::byte{'T'}, std::byte{0}};
    append_u32(out, 1);
    append_u32(out, static_cast<std::uint32_t>(AssetType::StaticMesh));
    append_string(out, "asset.legacy");
    append_string(out, "legacy-fingerprint");
    append_u32(out, 1);
    append_string(out, "asset.dependency");
    append_string(out, "Art/Source/Legacy.blend");
    append_string(out, "Art/Exports/Legacy.glb.asset.json");
    append_u64(out, 2);
    out.push_back(std::byte{7});
    out.push_back(std::byte{9});
    return out;
}
}

HH_TEST("hasset serialization is byte identical and v2 round trips policy metadata") {
    const auto document = sample();
    const auto a = serialize_hasset(document);
    const auto b = serialize_hasset(document);
    HH_REQUIRE(a == b);
    const auto parsed = parse_hasset(a);
    HH_REQUIRE(parsed.type == AssetType::StaticMesh);
    HH_REQUIRE(parsed.asset_id == document.asset_id);
    HH_REQUIRE(parsed.fingerprint == document.fingerprint);
    HH_REQUIRE(parsed.dependencies == std::vector<std::string>({"asset.a", "asset.z"}));
    HH_REQUIRE(parsed.source_path == document.source_path);
    HH_REQUIRE(parsed.sidecar_path == document.sidecar_path);
    HH_REQUIRE(parsed.units == "meters");
    HH_REQUIRE(parsed.lod_policy == "prop_standard");
    HH_REQUIRE(parsed.collision_policy == "simple_authored");
    HH_REQUIRE(parsed.cutaway_policy == "fade_when_foreground");
    HH_REQUIRE(parsed.pivot_profile == "floor_contact_center");
    HH_REQUIRE(parsed.interaction_anchors == std::vector<std::string>({"INT_REPAIR_01", "INT_USE_01"}));
    HH_REQUIRE(parsed.payload == document.payload);
}
HH_TEST("hasset v1 remains readable with empty v2 policy fields") {
    const auto parsed = parse_hasset(legacy_v1_fixture());
    HH_REQUIRE(parsed.type == AssetType::StaticMesh);
    HH_REQUIRE(parsed.asset_id == "asset.legacy");
    HH_REQUIRE(parsed.fingerprint == "legacy-fingerprint");
    HH_REQUIRE(parsed.dependencies == std::vector<std::string>({"asset.dependency"}));
    HH_REQUIRE(parsed.source_path == "Art/Source/Legacy.blend");
    HH_REQUIRE(parsed.sidecar_path == "Art/Exports/Legacy.glb.asset.json");
    HH_REQUIRE(parsed.units.empty());
    HH_REQUIRE(parsed.lod_policy.empty());
    HH_REQUIRE(parsed.collision_policy.empty());
    HH_REQUIRE(parsed.cutaway_policy.empty());
    HH_REQUIRE(parsed.pivot_profile.empty());
    HH_REQUIRE(parsed.interaction_anchors.empty());
    HH_REQUIRE(parsed.payload == std::vector<std::byte>({std::byte{7}, std::byte{9}}));
}
HH_TEST("hasset rejects bad magic") {
    auto bytes = serialize_hasset(sample());
    bytes[0] = std::byte{'X'};
    HH_REQUIRE(parse_throws(bytes));
}
HH_TEST("hasset rejects unsupported version") {
    auto bytes = serialize_hasset(sample());
    bytes[8] = std::byte{3};
    HH_REQUIRE(parse_throws(bytes));
}
HH_TEST("hasset rejects truncation at every suffix boundary") {
    const auto bytes = serialize_hasset(sample());
    for (std::size_t size = 0; size < bytes.size(); ++size) {
        std::vector<std::byte> truncated(bytes.begin(), bytes.begin() + static_cast<std::ptrdiff_t>(size));
        HH_REQUIRE(parse_throws(truncated));
    }
}
HH_TEST("hasset serializer rejects absolute provenance paths") {
    auto document = sample();
#ifdef _WIN32
    document.source_path = "C:/work/Art/Source/Bed.blend";
#else
    document.source_path = "/work/Art/Source/Bed.blend";
#endif
    bool threw = false;
    try { static_cast<void>(serialize_hasset(document)); } catch (const std::exception&) { threw = true; }
    HH_REQUIRE(threw);
}
