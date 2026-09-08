#include "Test.h"
#include "hh/assets/Catalog.h"
#include "hh/assets/Cooker.h"
#include "hh/assets/DependencyGraph.h"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>

using namespace hh::assets;
namespace fs = std::filesystem;

#ifndef _WIN32
namespace {

fs::path make_repo() {
    static int serial = 0;
    const auto root = fs::temp_directory_path() /
        ("hh_cooker_atomic_" + std::to_string(++serial));
    fs::remove_all(root);
    fs::create_directories(root / "Art/Exports");
    fs::create_directories(root / "Art/Source");
    return root;
}

void write_text(const fs::path& path, std::string_view text) {
    fs::create_directories(path.parent_path());
    std::ofstream(path, std::ios::binary | std::ios::trunc) << text;
}

std::string read_text(const fs::path& path) {
    std::ifstream input(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

void add_asset(const fs::path& root) {
    write_text(root / "Art/Source/a.blend", "source-asset.a");
    write_text(root / "Art/Exports/a.glb", "export-asset.a");
    write_text(
        root / "Art/Exports/a.glb.asset.json",
        "{\"schema\":1,\"asset_id\":\"asset.a\",\"asset_type\":\"StaticMeshAsset\","
        "\"source\":\"Art/Source/a.blend\",\"units\":\"meters\","
        "\"lod_policy\":\"prop_standard\",\"collision_policy\":\"simple_authored\","
        "\"material_slots\":[],\"tags\":[],\"dependencies\":[]}");
}

CookOptions options(const fs::path& root) {
    return {
        root,
        root / "Build/CookedAssets",
        {"importer-1", "cooker-1", "none", "windows-x64"},
    };
}

}

HH_TEST("cooker rejects cooked asset temp symlink escapes before writing") {
    const auto root = make_repo();
    add_asset(root);
    const auto cooked = root / "Build/CookedAssets";
    fs::create_directories(cooked);

    const auto outside = root.parent_path() / (root.filename().string() + "_outside_asset_tmp");
    fs::remove_all(outside);
    fs::create_directories(outside);
    const auto victim = outside / "victim.bin";
    write_text(victim, "sentinel-asset-temp");
    const auto before = read_text(victim);
    fs::create_symlink(victim, cooked / "asset.a.hasset.tmp");

    const auto catalog = AssetCatalog::scan(root / "Art/Exports");
    const auto graph = DependencyGraph::build(catalog);
    bool threw = false;
    try {
        static_cast<void>(cook_one(catalog, graph, "asset.a", options(root)));
    } catch (const std::exception&) {
        threw = true;
    }

    HH_REQUIRE(threw);
    HH_REQUIRE(read_text(victim) == before);
    HH_REQUIRE(!fs::exists(cooked / "asset.a.hasset"));
    fs::remove_all(outside);
}

HH_TEST("cooker rejects cook state temp symlink escapes before writing") {
    const auto root = make_repo();
    add_asset(root);
    const auto cooked = root / "Build/CookedAssets";
    fs::create_directories(cooked);

    const auto outside = root.parent_path() / (root.filename().string() + "_outside_state_tmp");
    fs::remove_all(outside);
    fs::create_directories(outside);
    const auto victim = outside / "victim.json";
    write_text(victim, "sentinel-state-temp");
    const auto before = read_text(victim);
    fs::create_symlink(victim, cooked / ".cook-state.json.tmp");

    const auto catalog = AssetCatalog::scan(root / "Art/Exports");
    const auto graph = DependencyGraph::build(catalog);
    bool threw = false;
    try {
        static_cast<void>(cook_one(catalog, graph, "asset.a", options(root)));
    } catch (const std::exception&) {
        threw = true;
    }

    HH_REQUIRE(threw);
    HH_REQUIRE(read_text(victim) == before);
    HH_REQUIRE(!fs::exists(cooked / ".cook-state.json"));
    fs::remove_all(outside);
}
#endif
