#include "Test.h"
#include "hh/assets/Catalog.h"
#include "hh/assets/DependencyGraph.h"
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

using namespace hh::assets;
namespace fs = std::filesystem;
namespace {
fs::path make_repo() {
    static int serial = 0;
    auto root = fs::temp_directory_path() / ("hh_catalog_" + std::to_string(++serial));
    fs::remove_all(root);
    fs::create_directories(root / "Art/Exports");
    fs::create_directories(root / "Art/Source");
    return root;
}
void write_bytes(const fs::path& p, std::string_view s) { fs::create_directories(p.parent_path()); std::ofstream(p, std::ios::binary) << s; }
void add_asset(const fs::path& root, std::string id, std::string name, std::string deps = "[]") {
    write_bytes(root / ("Art/Source/" + name + ".blend"), "source-" + id);
    write_bytes(root / ("Art/Exports/" + name + ".glb"), "export-" + id);
    std::ofstream out(root / ("Art/Exports/" + name + ".glb.asset.json"), std::ios::binary);
    out << "{\"schema\":1,\"asset_id\":\"" << id << "\",\"asset_type\":\"StaticMeshAsset\","
        << "\"source\":\"Art/Source/" << name << ".blend\",\"units\":\"meters\","
        << "\"lod_policy\":\"prop_standard\",\"collision_policy\":\"simple_authored\","
        << "\"material_slots\":[],\"tags\":[],\"dependencies\":" << deps << "}";
}
}

HH_TEST("catalog scans sidecars and resolves stable IDs") {
    const auto root = make_repo();
    add_asset(root, "asset.b", "b");
    add_asset(root, "asset.a", "a", "[\"asset.b\"]");
    const auto catalog = AssetCatalog::scan(root / "Art/Exports");
    HH_REQUIRE(catalog.size() == 2);
    HH_REQUIRE(catalog.by_id("asset.a").metadata.asset_id == "asset.a");
    HH_REQUIRE(catalog.by_id("asset.a").export_path.filename() == "a.glb");
    HH_REQUIRE(catalog.by_id("asset.a").source_path == root / "Art/Source/a.blend");
}

HH_TEST("catalog ignores transient rsync sidecars") {
    const auto root = make_repo();
    add_asset(root, "asset.a", "a");
    const auto stable = root / "Art/Exports/a.glb.asset.json";
    const auto transient = root / "Art/Exports/Batch01/.rsync-tmp/a.glb.asset.json";
    fs::create_directories(transient.parent_path());
    fs::copy_file(stable, transient);

    const auto catalog = AssetCatalog::scan(root / "Art/Exports");
    HH_REQUIRE(catalog.size() == 1);
    HH_REQUIRE(catalog.by_id("asset.a").sidecar_path == stable);
}

HH_TEST("catalog ignores a shadowed legacy sidecar alias") {
    const auto root = make_repo();
    add_asset(root, "asset.a", "a");
    const auto stable = root / "Art/Exports/a.glb.asset.json";
    fs::copy_file(stable, root / "Art/Exports/a.asset.json");

    const auto catalog = AssetCatalog::scan(root / "Art/Exports");
    HH_REQUIRE(catalog.size() == 1);
    HH_REQUIRE(catalog.by_id("asset.a").sidecar_path == stable);
}

HH_TEST("catalog rejects incompatible shadowed legacy sidecar aliases") {
    for (const auto& conflict : std::vector<std::pair<std::string, std::string>>{
             {"asset_id", "asset.other"}, {"source", "Art/Source/other.blend"},
             {"dependencies", "[\"asset.missing\"]"}}) {
        const auto root = make_repo();
        add_asset(root, "asset.a", "a");
        const auto stable = root / "Art/Exports/a.glb.asset.json";
        std::ifstream input(stable, std::ios::binary);
        const std::string original{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
        std::string legacy = original;
        if (conflict.first == "asset_id") {
            legacy.replace(legacy.find("\"asset_id\":\"asset.a\""),
                          std::string("\"asset_id\":\"asset.a\"").size(),
                          "\"asset_id\":\"asset.other\"");
        } else if (conflict.first == "source") {
            legacy.replace(legacy.find("Art/Source/a.blend"), std::string("Art/Source/a.blend").size(), conflict.second);
        } else {
            legacy.replace(legacy.find("\"dependencies\":[]"), std::string("\"dependencies\":[]").size(),
                          "\"dependencies\":" + conflict.second);
        }
        write_bytes(root / "Art/Exports/a.asset.json", legacy);

        bool threw = false;
        try { static_cast<void>(AssetCatalog::scan(root / "Art/Exports")); }
        catch (const std::exception&) { threw = true; }
        HH_REQUIRE(threw);
    }
}

HH_TEST("catalog does not treat a dotted sibling stem as a legacy alias") {
    const auto root = make_repo();
    add_asset(root, "asset.dotted", "cart.wheels");
    write_bytes(root / "Art/Source/cart.blend", "source-alias");
    write_bytes(root / "Art/Exports/cart.asset.json",
        "{\"schema\":1,\"asset_id\":\"asset.alias\",\"asset_type\":\"StaticMeshAsset\","
        "\"source\":\"Art/Source/cart.blend\",\"units\":\"meters\",\"lod_policy\":\"prop_standard\","
        "\"collision_policy\":\"simple_authored\",\"material_slots\":[],\"tags\":[],\"dependencies\":[]}");

    bool threw = false;
    try { static_cast<void>(AssetCatalog::scan(root / "Art/Exports")); }
    catch (const std::exception&) { threw = true; }
    HH_REQUIRE(threw);
}

HH_TEST("catalog rejects duplicate logical IDs") {
    const auto root = make_repo();
    add_asset(root, "asset.same", "a");
    add_asset(root, "asset.same", "b");
    bool threw = false;
    try { static_cast<void>(AssetCatalog::scan(root / "Art/Exports")); } catch (const std::exception&) { threw = true; }
    HH_REQUIRE(threw);
}

HH_TEST("dependency graph returns deterministic transitive relationships") {
    const auto root = make_repo();
    add_asset(root, "asset.c", "c");
    add_asset(root, "asset.b", "b", "[\"asset.c\"]");
    add_asset(root, "asset.a", "a", "[\"asset.b\"]");
    const auto catalog = AssetCatalog::scan(root / "Art/Exports");
    const auto graph = DependencyGraph::build(catalog);
    HH_REQUIRE(graph.dependencies_of("asset.a", true) == std::vector<std::string>({"asset.b", "asset.c"}));
    HH_REQUIRE(graph.dependents_of("asset.c", true) == std::vector<std::string>({"asset.a", "asset.b"}));
    HH_REQUIRE(graph.topological_order() == std::vector<std::string>({"asset.c", "asset.b", "asset.a"}));
}

HH_TEST("dependency graph rejects missing dependency") {
    const auto root = make_repo();
    add_asset(root, "asset.a", "a", "[\"asset.missing\"]");
    const auto catalog = AssetCatalog::scan(root / "Art/Exports");
    bool threw = false;
    try { static_cast<void>(DependencyGraph::build(catalog)); } catch (const std::exception&) { threw = true; }
    HH_REQUIRE(threw);
}

HH_TEST("dependency graph rejects cycles") {
    const auto root = make_repo();
    add_asset(root, "asset.a", "a", "[\"asset.b\"]");
    add_asset(root, "asset.b", "b", "[\"asset.a\"]");
    const auto catalog = AssetCatalog::scan(root / "Art/Exports");
    bool threw = false;
    try { static_cast<void>(DependencyGraph::build(catalog)); } catch (const std::exception&) { threw = true; }
    HH_REQUIRE(threw);
}

HH_TEST("equal-order graph nodes are lexically deterministic") {
    const auto root = make_repo();
    add_asset(root, "asset.z", "z");
    add_asset(root, "asset.a", "a");
    const auto graph = DependencyGraph::build(AssetCatalog::scan(root / "Art/Exports"));
    HH_REQUIRE(graph.topological_order() == std::vector<std::string>({"asset.a", "asset.z"}));
}
