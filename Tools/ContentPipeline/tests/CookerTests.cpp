#include "Test.h"
#include "hh/assets/Catalog.h"
#include "hh/assets/Cooker.h"
#include "hh/assets/DependencyGraph.h"
#include "hh/assets/Json.h"
#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

using namespace hh::assets;
namespace fs = std::filesystem;
namespace {
fs::path make_repo() {
    static int serial = 0;
    auto root = fs::temp_directory_path() / ("hh_cooker_" + std::to_string(++serial));
    fs::remove_all(root);
    fs::create_directories(root / "Art/Exports");
    fs::create_directories(root / "Art/Source");
    return root;
}
void write_text(const fs::path& p, std::string_view s) { fs::create_directories(p.parent_path()); std::ofstream(p, std::ios::binary) << s; }
std::string read_text_file(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}
std::string json_string(std::string_view value) {
    std::string out{"\""};
    for (const unsigned char c : value) {
        switch (c) {
        case '"': out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\b': out += "\\b"; break;
        case '\f': out += "\\f"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:
            if (c < 0x20u) {
                constexpr char hex[] = "0123456789ABCDEF";
                out += "\\u00";
                out.push_back(hex[(c >> 4u) & 0x0Fu]);
                out.push_back(hex[c & 0x0Fu]);
            } else {
                out.push_back(static_cast<char>(c));
            }
        }
    }
    out.push_back('"');
    return out;
}
void add_asset(const fs::path& root, std::string id, std::string name, std::string deps = "[]") {
    write_text(root / ("Art/Source/" + name + ".blend"), "source-" + id);
    write_text(root / ("Art/Exports/" + name + ".glb"), "export-" + id);
    std::ofstream out(root / ("Art/Exports/" + name + ".glb.asset.json"), std::ios::binary);
    out << "{\"schema\":1,\"asset_id\":" << json_string(id) << ",\"asset_type\":\"StaticMeshAsset\","
        << "\"source\":" << json_string("Art/Source/" + name + ".blend") << ",\"units\":\"meters\","
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
HH_TEST("failed recook preserves previous valid output") {
    const auto root = make_repo(); add_asset(root, "asset.a", "a");
    auto catalog = AssetCatalog::scan(root / "Art/Exports"); auto graph = DependencyGraph::build(catalog);
    const auto first = cook_one(catalog, graph, "asset.a", options(root));
    const auto before = read_bytes(first.output);
    fs::remove(root / "Art/Exports/a.glb");
    bool threw = false; try { static_cast<void>(cook_one(catalog, graph, "asset.a", options(root))); } catch (const std::exception&) { threw = true; }
    HH_REQUIRE(threw); HH_REQUIRE(read_bytes(first.output) == before); HH_REQUIRE(!fs::exists(first.output.string() + ".tmp"));
}
HH_TEST("corrupted cooked output is recooked even when fingerprint state matches") {
    const auto root = make_repo(); add_asset(root, "asset.a", "a");
    const auto catalog = AssetCatalog::scan(root / "Art/Exports"); const auto graph = DependencyGraph::build(catalog);
    const auto first = cook_one(catalog, graph, "asset.a", options(root));
    HH_REQUIRE(first.cooked);
    write_text(first.output, "corrupt-hasset");
    const auto corrupted = read_bytes(first.output);
    const auto second = cook_one(catalog, graph, "asset.a", options(root));
    HH_REQUIRE(second.cooked);
    HH_REQUIRE(read_bytes(second.output) != corrupted);
}
HH_TEST("cook state remains valid JSON when stale entries contain control characters") {
    const auto root = make_repo(); add_asset(root, "asset.a", "a");
    const auto state = root / "Build/CookedAssets/.cook-state.json";
    write_text(state, "{\"assets\":{\"stale\\u0001id\":\"deadbeef\"},\"schema\":1}");
    const auto catalog = AssetCatalog::scan(root / "Art/Exports"); const auto graph = DependencyGraph::build(catalog);
    HH_REQUIRE(cook_one(catalog, graph, "asset.a", options(root)).cooked);
    const auto parsed = parse_json(read_text_file(state));
    HH_REQUIRE(parsed.is_object());
}
HH_TEST("malformed cook state fails closed without damaging previous cooked output") {
    const auto root = make_repo(); add_asset(root, "asset.a", "a");
    const auto catalog = AssetCatalog::scan(root / "Art/Exports"); const auto graph = DependencyGraph::build(catalog);
    const auto first = cook_one(catalog, graph, "asset.a", options(root));
    const auto before = read_bytes(first.output);
    write_text(root / "Build/CookedAssets/.cook-state.json", "{\"schema\":1,\"assets\":{");
    bool threw = false;
    try { static_cast<void>(cook_one(catalog, graph, "asset.a", options(root))); } catch (const std::exception&) { threw = true; }
    HH_REQUIRE(threw);
    HH_REQUIRE(read_bytes(first.output) == before);
    HH_REQUIRE(!fs::exists(root / "Build/CookedAssets/.cook-state.json.tmp"));
}
HH_TEST("cooker rejects cooked roots outside repository") {
    const auto root = make_repo(); add_asset(root, "asset.a", "a");
    const auto outside = root.parent_path() / (root.filename().string() + "_outside_cooked");
    fs::remove_all(outside);
    auto opts = options(root);
    opts.cooked_root = outside;
    const auto catalog = AssetCatalog::scan(root / "Art/Exports"); const auto graph = DependencyGraph::build(catalog);
    bool threw = false;
    try { static_cast<void>(cook_one(catalog, graph, "asset.a", opts)); } catch (const std::exception&) { threw = true; }
    HH_REQUIRE(threw);
    HH_REQUIRE(!fs::exists(outside / "asset.a.hasset"));
    fs::remove_all(outside);
}
#ifndef _WIN32
HH_TEST("cooker rejects cooked root symlinks resolving outside repository") {
    const auto root = make_repo(); add_asset(root, "asset.a", "a");
    const auto outside = root.parent_path() / (root.filename().string() + "_outside_cooked_link");
    fs::remove_all(outside);
    fs::create_directories(outside);
    fs::create_directories(root / "Build");
    fs::create_directory_symlink(outside, root / "Build/CookedAssets");
    const auto catalog = AssetCatalog::scan(root / "Art/Exports"); const auto graph = DependencyGraph::build(catalog);
    bool threw = false;
    try { static_cast<void>(cook_one(catalog, graph, "asset.a", options(root))); } catch (const std::exception&) { threw = true; }
    HH_REQUIRE(threw);
    HH_REQUIRE(!fs::exists(outside / "asset.a.hasset"));
    fs::remove_all(outside);
}
#endif
HH_TEST("asset IDs cannot become cooked filesystem paths") {
    const auto root = make_repo(); add_asset(root, "asset/bad", "bad");
    const auto catalog = AssetCatalog::scan(root / "Art/Exports"); const auto graph = DependencyGraph::build(catalog);
    bool threw = false; try { static_cast<void>(cook_one(catalog, graph, "asset/bad", options(root))); } catch (const std::exception&) { threw = true; }
    HH_REQUIRE(threw);
}
HH_TEST("asset IDs reject Windows reserved filename characters on every platform") {
    constexpr const char* invalid_ids[] = {"asset:bad", "asset*bad", "asset?bad", "asset\"bad", "asset<bad", "asset>bad", "asset|bad"};
    for (const char* id : invalid_ids) {
        const auto root = make_repo(); add_asset(root, id, "bad");
        const auto catalog = AssetCatalog::scan(root / "Art/Exports"); const auto graph = DependencyGraph::build(catalog);
        bool threw = false; try { static_cast<void>(cook_one(catalog, graph, id, options(root))); } catch (const std::exception&) { threw = true; }
        HH_REQUIRE(threw);
    }
}
HH_TEST("asset IDs reject Windows device names and trailing aliases on every platform") {
    constexpr const char* invalid_ids[] = {"CON", "PRN", "AUX", "NUL", "COM1", "LPT9", "asset.bad.", "asset.bad "};
    for (const char* id : invalid_ids) {
        const auto root = make_repo(); add_asset(root, id, "bad");
        const auto catalog = AssetCatalog::scan(root / "Art/Exports"); const auto graph = DependencyGraph::build(catalog);
        bool threw = false; try { static_cast<void>(cook_one(catalog, graph, id, options(root))); } catch (const std::exception&) { threw = true; }
        HH_REQUIRE(threw);
    }
}
