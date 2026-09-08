#include "Test.h"
#include "hh/assets/Hasset.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <new>
#include <stdexcept>
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
void overwrite_u32(std::vector<std::byte>& bytes, std::size_t offset, std::uint32_t value) {
    for (unsigned shift = 0; shift < 32u; shift += 8u) {
        bytes[offset++] = static_cast<std::byte>((value >> shift) & 0xffu);
    }
}
std::size_t dependency_count_offset(const HassetDocument& d) {
    return 8u + 4u + 4u + 4u + d.asset_id.size() + 4u + d.fingerprint.size();
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
HH_TEST("hasset serializer rejects provenance traversal") {
    auto document = sample();
    document.source_path = "../outside/Bed.blend";
    bool threw = false;
    try { static_cast<void>(serialize_hasset(document)); } catch (const std::exception&) { threw = true; }
    HH_REQUIRE(threw);
}
HH_TEST("hasset parser rejects traversal embedded in provenance") {
    const auto document = sample();
    auto bytes = serialize_hasset(document);
    const std::string needle = document.source_path;
    const auto first = std::search(bytes.begin(), bytes.end(),
        reinterpret_cast<const std::byte*>(needle.data()),
        reinterpret_cast<const std::byte*>(needle.data() + needle.size()));
    HH_REQUIRE(first != bytes.end());
    *first = std::byte{'.'};
    *(first + 1) = std::byte{'.'};
    *(first + 2) = std::byte{'/'};
    HH_REQUIRE(parse_throws(bytes));
}
HH_TEST("hasset parser rejects impossible dependency count before allocation") {
    const auto document = sample();
    auto bytes = serialize_hasset(document);
    overwrite_u32(bytes, dependency_count_offset(document), 0xffffffffu);
    bool rejected = false;
    bool allocation_failure = false;
    try {
        static_cast<void>(parse_hasset(bytes));
    } catch (const std::bad_alloc&) {
        allocation_failure = true;
    } catch (const std::length_error&) {
        allocation_failure = true;
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    HH_REQUIRE(rejected);
    HH_REQUIRE(!allocation_failure);
}
