#include "Test.h"
#include "hh/assets/Catalog.h"
#include "hh/assets/DependencyGraph.h"
#include "hh/assets/Fingerprint.h"
#include "hh/assets/Hash.h"
#include <array>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
#include <vector>

using namespace hh::assets;
namespace fs = std::filesystem;
namespace {
fs::path make_repo() {
    static int serial = 0;
    auto root = fs::temp_directory_path() / ("hh_fingerprint_" + std::to_string(++serial));
    fs::remove_all(root);
    fs::create_directories(root / "Art/Exports");
    fs::create_directories(root / "Art/Source");
    return root;
}
void write_text(const fs::path& p, std::string_view s) { fs::create_directories(p.parent_path()); std::ofstream(p, std::ios::binary) << s; }
std::string read_text(const fs::path& p) { std::ifstream in(p, std::ios::binary); return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()); }
void add_asset(const fs::path& root, std::string id, std::string name, std::string deps = "[]") {
    write_text(root / ("Art/Source/" + name + ".blend"), "source-" + id);
    write_text(root / ("Art/Exports/" + name + ".glb"), "export-" + id);
    std::ofstream out(root / ("Art/Exports/" + name + ".glb.asset.json"), std::ios::binary);
    out << "{\"schema\":1,\"asset_id\":\"" << id << "\",\"asset_type\":\"StaticMeshAsset\","
        << "\"source\":\"Art/Source/" << name << ".blend\",\"units\":\"meters\","
        << "\"lod_policy\":\"prop_standard\",\"collision_policy\":\"simple_authored\","
        << "\"material_slots\":[],\"tags\":[],\"dependencies\":" << deps << "}";
}
FingerprintSettings settings() { return {"importer-1", "cooker-1", "none", "windows-x64"}; }
std::string fingerprint(const fs::path& root, std::string_view id, const FingerprintSettings& s = settings()) {
    const auto catalog = AssetCatalog::scan(root / "Art/Exports");
    const auto graph = DependencyGraph::build(catalog);
    return compute_fingerprint(catalog.by_id(id), catalog, graph, s);
}
}

HH_TEST("sha256 matches standard abc vector") {
    const std::string text = "abc";
    const auto bytes = std::as_bytes(std::span(text.data(), text.size()));
    HH_REQUIRE(sha256_hex(bytes) == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}
HH_TEST("timestamp-only changes do not affect fingerprint") {
    const auto root = make_repo(); add_asset(root, "asset.a", "a");
    const auto before = fingerprint(root, "asset.a");
    fs::last_write_time(root / "Art/Source/a.blend", fs::file_time_type::clock::now() + std::chrono::hours(2));
    fs::last_write_time(root / "Art/Exports/a.glb.asset.json", fs::file_time_type::clock::now() + std::chrono::hours(3));
    HH_REQUIRE(fingerprint(root, "asset.a") == before);
}
HH_TEST("source sidecar and export bytes each invalidate fingerprint") {
    const auto root = make_repo(); add_asset(root, "asset.a", "a");
    const auto base = fingerprint(root, "asset.a");
    write_text(root / "Art/Source/a.blend", "changed-source");
    const auto source_changed = fingerprint(root, "asset.a");
    HH_REQUIRE(source_changed != base);
    write_text(root / "Art/Source/a.blend", "source-asset.a");
    auto sidecar = read_text(root / "Art/Exports/a.glb.asset.json");
    write_text(root / "Art/Exports/a.glb.asset.json", sidecar + "\n");
    HH_REQUIRE(fingerprint(root, "asset.a") != base);
    write_text(root / "Art/Exports/a.glb.asset.json", sidecar);
    write_text(root / "Art/Exports/a.glb", "changed-export");
    HH_REQUIRE(fingerprint(root, "asset.a") != base);
}
HH_TEST("transitive dependency content invalidates dependent fingerprint") {
    const auto root = make_repo();
    add_asset(root, "asset.c", "c");
    add_asset(root, "asset.b", "b", "[\"asset.c\"]");
    add_asset(root, "asset.a", "a", "[\"asset.b\"]");
    const auto base = fingerprint(root, "asset.a");
    write_text(root / "Art/Source/c.blend", "changed-c");
    HH_REQUIRE(fingerprint(root, "asset.a") != base);
}
HH_TEST("tool and platform settings invalidate fingerprint independently") {
    const auto root = make_repo(); add_asset(root, "asset.a", "a");
    const auto base = fingerprint(root, "asset.a");
    auto s = settings(); s.importer_version = "importer-2"; HH_REQUIRE(fingerprint(root, "asset.a", s) != base);
    s = settings(); s.cooker_version = "cooker-2"; HH_REQUIRE(fingerprint(root, "asset.a", s) != base);
    s = settings(); s.compression_settings = "deterministic-zstd"; HH_REQUIRE(fingerprint(root, "asset.a", s) != base);
    s = settings(); s.platform_target = "windows-arm64"; HH_REQUIRE(fingerprint(root, "asset.a", s) != base);
}
