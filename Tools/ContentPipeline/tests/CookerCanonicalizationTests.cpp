#include "Test.h"
#include "hh/assets/Catalog.h"
#include "hh/assets/Cooker.h"
#include "hh/assets/DependencyGraph.h"

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

using namespace hh::assets;
namespace fs = std::filesystem;
namespace {

fs::path make_repo() {
    static int serial = 0;
    auto root = fs::temp_directory_path() / ("hh_cooker_canonical_" + std::to_string(++serial));
    fs::remove_all(root);
    fs::create_directories(root / "Art/Exports");
    fs::create_directories(root / "Art/Source");
    return root;
}

void write_text(const fs::path& path, std::string_view text) {
    fs::create_directories(path.parent_path());
    std::ofstream(path, std::ios::binary | std::ios::trunc) << text;
}

void add_asset(const fs::path& root, std::string_view id, std::string_view name, std::string_view dependencies = "[]") {
    write_text(root / ("Art/Source/" + std::string(name) + ".blend"), "source-" + std::string(id));
    write_text(root / ("Art/Exports/" + std::string(name) + ".glb"), "export-" + std::string(id));
    std::ofstream out(root / ("Art/Exports/" + std::string(name) + ".glb.asset.json"), std::ios::binary | std::ios::trunc);
    out << "{\"schema\":1,\"asset_id\":\"" << id << "\",\"asset_type\":\"StaticMeshAsset\","
        << "\"source\":\"Art/Source/" << name << ".blend\",\"units\":\"meters\","
        << "\"lod_policy\":\"prop_standard\",\"collision_policy\":\"simple_authored\","
        << "\"material_slots\":[],\"tags\":[],\"dependencies\":" << dependencies << "}";
}

CookOptions options(const fs::path& root) {
    return {root, root / "Build/CookedAssets", {"importer-1", "cooker-1", "none", "windows-x64"}};
}

}  // namespace

HH_TEST("incremental cook treats dependency ordering as canonical") {
    const auto root = make_repo();
    add_asset(root, "asset.b", "b");
    add_asset(root, "asset.c", "c");
    add_asset(root, "asset.a", "a", "[\"asset.c\",\"asset.b\"]");

    const auto catalog = AssetCatalog::scan(root / "Art/Exports");
    const auto graph = DependencyGraph::build(catalog);
    const auto first = cook_one(catalog, graph, "asset.a", options(root));
    HH_REQUIRE(first.cooked);

    const auto second = cook_one(catalog, graph, "asset.a", options(root));
    HH_REQUIRE(!second.cooked);
    HH_REQUIRE(second.fingerprint == first.fingerprint);
}

HH_TEST("incremental cook treats duplicate dependency entries as canonical") {
    const auto root = make_repo();
    add_asset(root, "asset.b", "b");
    add_asset(root, "asset.a", "a", "[\"asset.b\",\"asset.b\"]");

    const auto catalog = AssetCatalog::scan(root / "Art/Exports");
    const auto graph = DependencyGraph::build(catalog);
    const auto first = cook_one(catalog, graph, "asset.a", options(root));
    HH_REQUIRE(first.cooked);

    const auto second = cook_one(catalog, graph, "asset.a", options(root));
    HH_REQUIRE(!second.cooked);
    HH_REQUIRE(second.fingerprint == first.fingerprint);
}
