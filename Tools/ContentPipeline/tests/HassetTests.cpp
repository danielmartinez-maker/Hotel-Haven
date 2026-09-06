#include "Test.h"
#include "hh/assets/Hasset.h"
#include <cstddef>
#include <filesystem>
#include <string>
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
    d.payload = {std::byte{1}, std::byte{2}, std::byte{3}, std::byte{255}};
    return d;
}
bool parse_throws(const std::vector<std::byte>& bytes) {
    try { static_cast<void>(parse_hasset(bytes)); return false; } catch (const std::exception&) { return true; }
}
}

HH_TEST("hasset serialization is byte identical and round trips") {
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
    HH_REQUIRE(parsed.payload == document.payload);
}
HH_TEST("hasset rejects bad magic") {
    auto bytes = serialize_hasset(sample());
    bytes[0] = std::byte{'X'};
    HH_REQUIRE(parse_throws(bytes));
}
HH_TEST("hasset rejects unsupported version") {
    auto bytes = serialize_hasset(sample());
    bytes[8] = std::byte{2};
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
