#include "Test.h"
#include "hh/assets/Cli.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

using namespace hh::assets;
namespace fs = std::filesystem;
namespace {
struct CurrentPathGuard { fs::path old = fs::current_path(); ~CurrentPathGuard() { fs::current_path(old); } };
fs::path make_repo() {
    static int serial = 0;
    auto root = fs::temp_directory_path() / ("hh_cli_" + std::to_string(++serial));
    fs::remove_all(root);
    fs::create_directories(root / "Art/Exports");
    fs::create_directories(root / "Art/Source");
    fs::create_directories(root / "Tools/ContentPipeline");
    return root;
}
void write_text(const fs::path& p, std::string_view s) { fs::create_directories(p.parent_path()); std::ofstream(p, std::ios::binary) << s; }
void add_asset(const fs::path& root, std::string id, std::string name, std::string deps = "[]", std::string extra = "") {
    write_text(root / ("Art/Source/" + name + ".blend"), "source-" + id);
    write_text(root / ("Art/Exports/" + name + ".glb"), "export-" + id);
    std::ofstream out(root / ("Art/Exports/" + name + ".glb.asset.json"), std::ios::binary);
    out << "{\"schema\":1,\"asset_id\":\"" << id << "\",\"asset_type\":\"StaticMeshAsset\","
        << "\"source\":\"Art/Source/" << name << ".blend\",\"units\":\"meters\","
        << "\"lod_policy\":\"prop_standard\",\"collision_policy\":\"simple_authored\","
        << "\"material_slots\":[],\"tags\":[],\"dependencies\":" << deps << extra << "}";
}
int run(const std::vector<std::string_view>& args, std::string& out_text, std::string& err_text) {
    std::ostringstream out, err;
    const int code = run_asset_cli(args, out, err);
    out_text = out.str(); err_text = err.str(); return code;
}
void unset_blender() {
#ifdef _WIN32
    _putenv_s("HOTEL_HAVEN_BLENDER", "");
#else
    unsetenv("HOTEL_HAVEN_BLENDER");
#endif
}
}

HH_TEST("CLI rejects malformed invocation") {
    const auto root = make_repo(); CurrentPathGuard guard; fs::current_path(root);
    std::string out, err;
    HH_REQUIRE(run({}, out, err) != 0);
    HH_REQUIRE(run({"cook"}, out, err) != 0);
    HH_REQUIRE(run({"audit", "vertical-slice"}, out, err) != 0);
}
HH_TEST("validate accepts exports directory and rejects invalid metadata") {
    const auto root = make_repo(); add_asset(root, "asset.a", "a"); CurrentPathGuard guard; fs::current_path(root);
    std::string out, err;
    HH_REQUIRE(run({"validate", "Art/Exports"}, out, err) == 0);
    HH_REQUIRE(out.find("validated 1 asset") != std::string::npos);
    auto text = std::string("{\"schema\":1,\"asset_id\":\"asset.bad\",\"asset_type\":\"StaticMeshAsset\",\"source\":\"Art/Source/a.blend\",\"units\":\"centimeters\",\"lod_policy\":\"p\",\"collision_policy\":\"c\",\"material_slots\":[],\"tags\":[],\"dependencies\":[]}");
    write_text(root / "Art/Exports/bad.glb", "bad"); write_text(root / "Art/Exports/bad.glb.asset.json", text);
    HH_REQUIRE(run({"validate", "Art/Exports"}, out, err) != 0);
}
HH_TEST("inspect and deps produce deterministic useful output") {
    const auto root = make_repo(); add_asset(root, "asset.b", "b"); add_asset(root, "asset.a", "a", "[\"asset.b\"]");
    CurrentPathGuard guard; fs::current_path(root); std::string out1, out2, err;
    HH_REQUIRE(run({"inspect", "asset.a"}, out1, err) == 0);
    HH_REQUIRE(run({"inspect", "asset.a"}, out2, err) == 0);
    HH_REQUIRE(out1 == out2); HH_REQUIRE(out1.find("asset.a") != std::string::npos); HH_REQUIRE(out1.find("fingerprint") != std::string::npos);
    HH_REQUIRE(run({"deps", "asset.a"}, out1, err) == 0);
    HH_REQUIRE(out1.find("direct_dependencies: asset.b") != std::string::npos);
    HH_REQUIRE(out1.find("transitive_dependencies: asset.b") != std::string::npos);
}
HH_TEST("CLI cook one changed and all operate through same cooker") {
    const auto root = make_repo(); add_asset(root, "asset.b", "b"); add_asset(root, "asset.a", "a", "[\"asset.b\"]");
    CurrentPathGuard guard; fs::current_path(root); std::string out, err;
    HH_REQUIRE(run({"cook", "asset.a"}, out, err) == 0); HH_REQUIRE(fs::exists(root / "Build/CookedAssets/asset.a.hasset"));
    HH_REQUIRE(run({"cook", "--changed"}, out, err) == 0);
    HH_REQUIRE(run({"cook", "--all"}, out, err) == 0);
}
HH_TEST("milestone audit requires ownership reviewers release state and no blockers") {
    const auto root = make_repo();
    const std::string ready = ",\"content_owner\":\"art\",\"technical_reviewer\":\"tech\",\"art_reviewer\":\"lead\",\"dependent_feature_owner\":\"rooms\",\"milestone\":\"vertical-slice\",\"lifecycle_state\":\"RELEASE_READY\"";
    add_asset(root, "asset.ready", "ready", "[]", ready);
    CurrentPathGuard guard; fs::current_path(root); std::string out, err;
    HH_REQUIRE(run({"audit", "--milestone", "vertical-slice"}, out, err) == 0);
    add_asset(root, "asset.notready", "notready", "[]", ",\"milestone\":\"vertical-slice\",\"lifecycle_state\":\"APPROVED\"");
    HH_REQUIRE(run({"audit", "--milestone", "vertical-slice"}, out, err) != 0);
}
HH_TEST("export reports deterministic configuration failure when Blender is unavailable") {
    const auto root = make_repo(); add_asset(root, "asset.a", "a"); CurrentPathGuard guard; fs::current_path(root); unset_blender();
    std::string out, err; HH_REQUIRE(run({"export", "asset.a"}, out, err) != 0); HH_REQUIRE(err.find("HOTEL_HAVEN_BLENDER") != std::string::npos);
}
