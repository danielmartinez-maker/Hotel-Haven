#include "Test.h"
#include "hh/assets/Catalog.h"
#include "hh/assets/Cooker.h"
#include "hh/assets/DependencyGraph.h"
#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

namespace hh::assets::test {
using CookStatePublicationObserver = void (*)(const std::filesystem::path&);
void set_cook_state_publication_observer(CookStatePublicationObserver observer) noexcept;
}

using namespace hh::assets;
namespace fs = std::filesystem;
namespace {
class CookStatePublicationProbe {
public:
    explicit CookStatePublicationProbe(std::vector<fs::path> expected_outputs)
        : expected_outputs_(std::move(expected_outputs)) {
        active_ = this;
        hh::assets::test::set_cook_state_publication_observer(&observe);
    }
    ~CookStatePublicationProbe() {
        hh::assets::test::set_cook_state_publication_observer(nullptr);
        active_ = nullptr;
    }
    std::size_t publication_count() const noexcept { return publication_count_; }
    bool all_outputs_existed_on_publication() const noexcept { return all_outputs_existed_on_publication_; }

private:
    static void observe(const fs::path&) {
        ++active_->publication_count_;
        for (const auto& output : active_->expected_outputs_) {
            if (!fs::exists(output)) active_->all_outputs_existed_on_publication_ = false;
        }
    }

    inline static CookStatePublicationProbe* active_{};
    std::vector<fs::path> expected_outputs_;
    std::size_t publication_count_{};
    bool all_outputs_existed_on_publication_{true};
};

fs::path make_repo() {
    static int serial = 0;
    auto root = fs::temp_directory_path() / ("hh_cooker_" + std::to_string(++serial));
    fs::remove_all(root);
    fs::create_directories(root / "Art/Exports");
    fs::create_directories(root / "Art/Source");
    return root;
}
void write_text(const fs::path& p, std::string_view s) { fs::create_directories(p.parent_path()); std::ofstream(p, std::ios::binary) << s; }
void add_asset(const fs::path& root, std::string id, std::string name, std::string deps = "[]") {
    write_text(root / ("Art/Source/" + name + ".blend"), "source-" + id);
    write_text(root / ("Art/Exports/" + name + ".glb"), "export-" + id);
    std::ofstream out(root / ("Art/Exports/" + name + ".glb.asset.json"), std::ios::binary);
    out << "{\"schema\":1,\"asset_id\":\"" << id << "\",\"asset_type\":\"StaticMeshAsset\","
        << "\"source\":\"Art/Source/" << name << ".blend\",\"units\":\"meters\","
        << "\"lod_policy\":\"prop_standard\",\"collision_policy\":\"simple_authored\","
        << "\"material_slots\":[],\"tags\":[],\"dependencies\":" << deps << "}";
}
CookOptions options(const fs::path& root) {
    return {root, root / "Build/CookedAssets", {"importer-1", "cooker-1", "none", "windows-x64"}};
}
std::vector<std::byte> read_bytes(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    std::vector<char> chars((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    std::vector<std::byte> out; out.reserve(chars.size()); for (char c : chars) out.push_back(static_cast<std::byte>(static_cast<unsigned char>(c))); return out;
}
std::size_t cooked_count(const std::vector<CookResult>& results) {
    return static_cast<std::size_t>(std::count_if(results.begin(), results.end(), [](const CookResult& r) { return r.cooked; }));
}
}

HH_TEST("first cook writes hasset and unchanged cook is a no-op") {
    const auto root = make_repo(); add_asset(root, "asset.a", "a");
    const auto catalog = AssetCatalog::scan(root / "Art/Exports"); const auto graph = DependencyGraph::build(catalog);
    const auto first = cook_one(catalog, graph, "asset.a", options(root));
    HH_REQUIRE(first.cooked); HH_REQUIRE(fs::exists(first.output));
    const auto second = cook_one(catalog, graph, "asset.a", options(root));
    HH_REQUIRE(!second.cooked); HH_REQUIRE(second.fingerprint == first.fingerprint);
}
HH_TEST("changed source recooks asset") {
    const auto root = make_repo(); add_asset(root, "asset.a", "a");
    auto catalog = AssetCatalog::scan(root / "Art/Exports"); auto graph = DependencyGraph::build(catalog);
    static_cast<void>(cook_one(catalog, graph, "asset.a", options(root)));
    write_text(root / "Art/Source/a.blend", "source changed");
    catalog = AssetCatalog::scan(root / "Art/Exports"); graph = DependencyGraph::build(catalog);
    HH_REQUIRE(cook_one(catalog, graph, "asset.a", options(root)).cooked);
}
HH_TEST("changed dependency recooks transitive dependent with cook changed") {
    const auto root = make_repo();
    add_asset(root, "asset.b", "b"); add_asset(root, "asset.a", "a", "[\"asset.b\"]");
    auto catalog = AssetCatalog::scan(root / "Art/Exports"); auto graph = DependencyGraph::build(catalog);
    HH_REQUIRE(cooked_count(cook_changed(catalog, graph, options(root))) == 2);
    HH_REQUIRE(cooked_count(cook_changed(catalog, graph, options(root))) == 0);
    write_text(root / "Art/Source/b.blend", "changed dependency");
    catalog = AssetCatalog::scan(root / "Art/Exports"); graph = DependencyGraph::build(catalog);
    HH_REQUIRE(cooked_count(cook_changed(catalog, graph, options(root))) == 2);
}
HH_TEST("cook all follows dependency-first deterministic order and is byte identical") {
    const auto root = make_repo();
    add_asset(root, "asset.b", "b"); add_asset(root, "asset.a", "a", "[\"asset.b\"]");
    const auto catalog = AssetCatalog::scan(root / "Art/Exports"); const auto graph = DependencyGraph::build(catalog);
    const auto first = cook_all(catalog, graph, options(root));
    HH_REQUIRE(first.size() == 2); HH_REQUIRE(first[0].asset_id == "asset.b"); HH_REQUIRE(first[1].asset_id == "asset.a");
    const auto before = read_bytes(first[1].output);
    const auto second = cook_all(catalog, graph, options(root));
    HH_REQUIRE(cooked_count(second) == 2);
    HH_REQUIRE(read_bytes(second[1].output) == before);
}
HH_TEST("multi-asset cook publishes state once after all outputs succeed") {
    const auto root = make_repo();
    add_asset(root, "asset.a", "a"); add_asset(root, "asset.b", "b");
    const auto catalog = AssetCatalog::scan(root / "Art/Exports"); const auto graph = DependencyGraph::build(catalog);
    CookStatePublicationProbe probe({root / "Build/CookedAssets/asset.a.hasset", root / "Build/CookedAssets/asset.b.hasset"});
    const auto results = cook_all(catalog, graph, options(root));
    HH_REQUIRE(results.size() == 2);
    if (probe.publication_count() != 1) {
        throw std::runtime_error("expected one cook-state publication, observed " + std::to_string(probe.publication_count()));
    }
    HH_REQUIRE(probe.all_outputs_existed_on_publication());
}
HH_TEST("failed multi-asset cook does not publish partial state") {
    const auto root = make_repo();
    add_asset(root, "asset.a", "a"); add_asset(root, "asset.b", "b");
    const auto catalog = AssetCatalog::scan(root / "Art/Exports"); const auto graph = DependencyGraph::build(catalog);
    fs::remove(root / "Art/Exports/b.glb");
    CookStatePublicationProbe probe({});
    bool threw = false;
    try { static_cast<void>(cook_all(catalog, graph, options(root))); } catch (const std::exception&) { threw = true; }
    HH_REQUIRE(threw); HH_REQUIRE(fs::exists(root / "Build/CookedAssets/asset.a.hasset"));
    HH_REQUIRE(probe.publication_count() == 0); HH_REQUIRE(!fs::exists(root / "Build/CookedAssets/.cook-state.json"));
}
HH_TEST("failed recook preserves previous valid output") {
    const auto root = make_repo(); add_asset(root, "asset.a", "a");
    auto catalog = AssetCatalog::scan(root / "Art/Exports"); auto graph = DependencyGraph::build(catalog);
    const auto first = cook_one(catalog, graph, "asset.a", options(root));
    const auto before = read_bytes(first.output);
    fs::remove(root / "Art/Exports/a.glb");
    bool threw = false; try { static_cast<void>(cook_one(catalog, graph, "asset.a", options(root))); } catch (const std::exception&) { threw = true; }
    HH_REQUIRE(threw); HH_REQUIRE(read_bytes(first.output) == before); HH_REQUIRE(!fs::exists(first.output.string() + ".tmp"));
}
HH_TEST("asset IDs cannot become cooked filesystem paths") {
    const auto root = make_repo(); add_asset(root, "asset/bad", "bad");
    const auto catalog = AssetCatalog::scan(root / "Art/Exports"); const auto graph = DependencyGraph::build(catalog);
    bool threw = false; try { static_cast<void>(cook_one(catalog, graph, "asset/bad", options(root))); } catch (const std::exception&) { threw = true; }
    HH_REQUIRE(threw);
}
