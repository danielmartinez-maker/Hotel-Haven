#include "Test.h"
#include "hh/assets/Catalog.h"
#include "hh/assets/Hasset.h"
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>

using namespace hh::assets;
namespace fs = std::filesystem;

namespace {
fs::path make_repo() {
    static int serial = 0;
    auto root = fs::temp_directory_path() / ("hh_portable_paths_" + std::to_string(++serial));
    fs::remove_all(root);
    fs::create_directories(root / "Art/Exports");
    fs::create_directories(root / "Art/Source");
    return root;
}

void write_text(const fs::path& path, std::string_view text) {
    fs::create_directories(path.parent_path());
    std::ofstream(path, std::ios::binary) << text;
}

void add_asset(const fs::path& root) {
    write_text(root / "Art/Source/a.blend", "source");
    write_text(root / "Art/Exports/a.glb", "export");
    write_text(
        root / "Art/Exports/a.glb.asset.json",
        "{\"schema\":1,\"asset_id\":\"asset.a\",\"asset_type\":\"StaticMeshAsset\","
        "\"source\":\"Art/Source/a.blend\",\"units\":\"meters\","
        "\"lod_policy\":\"prop_standard\",\"collision_policy\":\"simple_authored\","
        "\"material_slots\":[],\"tags\":[],\"dependencies\":[]}");
}

HassetDocument sample_hasset() {
    HassetDocument document;
    document.type = AssetType::StaticMesh;
    document.asset_id = "asset.a";
    document.fingerprint = "fingerprint";
    document.source_path = "Art/Source/a.blend";
    document.sidecar_path = "Art/Exports/a.glb.asset.json";
    return document;
}
}

HH_TEST("hasset rejects Windows drive-relative provenance on every platform") {
    constexpr const char* paths[] = {
        "C:Art/Source/a.blend",
        "d:relative\\asset.blend",
    };
    for (const char* path : paths) {
        auto document = sample_hasset();
        document.source_path = path;
        bool threw = false;
        try { static_cast<void>(serialize_hasset(document)); } catch (const std::exception&) { threw = true; }
        HH_REQUIRE(threw);
    }
}

HH_TEST("catalog rejects Windows drive-relative source paths on every platform") {
    constexpr const char* paths[] = {
        "C:Art/Source/a.blend",
        "d:relative\\asset.blend",
    };
    for (const char* source : paths) {
        const auto root = make_repo();
        write_text(root / "Art/Exports/a.glb", "export");
        std::ofstream out(root / "Art/Exports/a.glb.asset.json", std::ios::binary);
        out << "{\"schema\":1,\"asset_id\":\"asset.a\",\"asset_type\":\"StaticMeshAsset\","
            << "\"source\":\"";
        for (const char c : std::string_view(source)) {
            if (c == '\\') out << "\\\\";
            else out << c;
        }
        out << "\",\"units\":\"meters\","
            << "\"lod_policy\":\"prop_standard\",\"collision_policy\":\"simple_authored\","
            << "\"material_slots\":[],\"tags\":[],\"dependencies\":[]}";
        out.close();

        bool threw = false;
        try { static_cast<void>(AssetCatalog::scan(root / "Art/Exports")); } catch (const std::exception&) { threw = true; }
        HH_REQUIRE(threw);
    }
}

HH_TEST("catalog resolves repository-relative asset paths independent of process cwd") {
    const auto root = make_repo();
    add_asset(root);
    const auto catalog = AssetCatalog::scan(root / "Art/Exports");

    HH_REQUIRE(catalog.resolve("Art/Exports/a.glb").metadata.asset_id == "asset.a");
    HH_REQUIRE(catalog.resolve("Art/Exports/a.glb.asset.json").metadata.asset_id == "asset.a");
    HH_REQUIRE(catalog.resolve("Art/Source/a.blend").metadata.asset_id == "asset.a");
}
