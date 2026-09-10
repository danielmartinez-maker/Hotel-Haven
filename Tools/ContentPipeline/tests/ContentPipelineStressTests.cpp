#include "Test.h"
#include "hh/assets/Catalog.h"
#include "hh/assets/Cooker.h"
#include "hh/assets/DependencyGraph.h"
#include "hh/assets/Fingerprint.h"
#include "hh/assets/Hasset.h"
#include "hh/assets/Metadata.h"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using namespace hh::assets;
namespace fs = std::filesystem;

namespace {
std::size_t operationBudget() {
    const char* scale = std::getenv("HH_STRESS_SCALE");
    const std::string_view value = scale ? std::string_view{scale} : std::string_view{"pr"};
    if (value == "pr") return 10'000;
    if (value == "extended") return 100'000;
    if (value == "exhaustive") return 500'000;
    throw std::runtime_error("HH_STRESS_SCALE must be pr, extended, or exhaustive");
}

fs::path makeRepo(std::string_view suffix) {
    static std::uint64_t serial = 0;
    auto root = fs::temp_directory_path() /
                ("hh_content_stress_" + std::string(suffix) + "_" +
                 std::to_string(++serial));
    std::error_code ec;
    fs::remove_all(root, ec);
    fs::create_directories(root / "Art/Exports");
    fs::create_directories(root / "Art/Source");
    return root;
}

void writeText(const fs::path& path, std::string_view text) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out)
        throw std::runtime_error("failed to create stress fixture");
    out << text;
    if (!out)
        throw std::runtime_error("failed to write stress fixture");
}

void addAsset(const fs::path& root, const std::string& id,
              const std::string& name, const std::string& dependencies = "[]") {
    writeText(root / ("Art/Source/" + name + ".blend"), "source-" + id);
    writeText(root / ("Art/Exports/" + name + ".glb"), "export-" + id);
    std::ofstream out(root / ("Art/Exports/" + name + ".glb.asset.json"),
                      std::ios::binary | std::ios::trunc);
    if (!out)
        throw std::runtime_error("failed to create metadata fixture");
    out << "{\"schema\":1,\"asset_id\":\"" << id
        << "\",\"asset_type\":\"StaticMeshAsset\","
        << "\"source\":\"Art/Source/" << name
        << ".blend\",\"units\":\"meters\","
        << "\"lod_policy\":\"prop_standard\","
        << "\"collision_policy\":\"simple_authored\","
        << "\"material_slots\":[],\"tags\":[],\"dependencies\":"
        << dependencies << "}";
}

std::vector<std::byte> readBytes(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in)
        throw std::runtime_error("failed to read stress artifact");
    std::vector<char> chars((std::istreambuf_iterator<char>(in)),
                            std::istreambuf_iterator<char>());
    std::vector<std::byte> bytes;
    bytes.reserve(chars.size());
    for (unsigned char value : chars)
        bytes.push_back(static_cast<std::byte>(value));
    return bytes;
}

CookOptions options(const fs::path& root) {
    return {root,
            root / "Build/CookedAssets",
            {"stress-importer-1", "stress-cooker-1", "none", "windows-x64"}};
}

std::string dependencyJson(const std::string& id) {
    return "[\"" + id + "\"]";
}
}  // namespace

HH_TEST("content pipeline sustains large deterministic dependency graph queries") {
    const auto root = makeRepo("dag");
    constexpr std::size_t kAssets = 512;
    for (std::size_t i = 0; i < kAssets; ++i) {
        const auto id = "asset." + std::to_string(i);
        const auto name = "mesh_" + std::to_string(i);
        addAsset(root, id, name,
                 i == 0 ? "[]" : dependencyJson("asset." + std::to_string(i - 1)));
    }

    const auto catalog = AssetCatalog::scan(root / "Art/Exports");
    HH_REQUIRE(catalog.size() == kAssets);
    const auto graph = DependencyGraph::build(catalog);
    const auto order = graph.topological_order();
    HH_REQUIRE(order.size() == kAssets);
    HH_REQUIRE(order.front() == "asset.0");
    HH_REQUIRE(order.back() == "asset.511");

    const std::size_t operations = operationBudget();
    for (std::size_t i = 0; i < operations; ++i) {
        const std::size_t index = (i * 2654435761ULL) % kAssets;
        const auto id = "asset." + std::to_string(index);
        const auto direct = graph.dependencies_of(id, false);
        if (index == 0)
            HH_REQUIRE(direct.empty());
        else {
            HH_REQUIRE(direct.size() == 1);
            HH_REQUIRE(direct.front() == "asset." + std::to_string(index - 1));
        }
        if ((i % 1024u) == 0u) {
            const auto transitive = graph.dependencies_of(id, true);
            HH_REQUIRE(transitive.size() == index);
        }
    }

    std::error_code ec;
    fs::remove_all(root, ec);
}

HH_TEST("content pipeline rejects duplicate missing and cyclic graph corruption atomically") {
    {
        const auto root = makeRepo("duplicate");
        addAsset(root, "asset.same", "a");
        addAsset(root, "asset.same", "b");
        bool rejected = false;
        try {
            (void)AssetCatalog::scan(root / "Art/Exports");
        } catch (const std::exception&) {
            rejected = true;
        }
        HH_REQUIRE(rejected);
        std::error_code ec;
        fs::remove_all(root, ec);
    }
    {
        const auto root = makeRepo("missing");
        addAsset(root, "asset.a", "a", "[\"asset.missing\"]");
        const auto catalog = AssetCatalog::scan(root / "Art/Exports");
        bool rejected = false;
        try {
            (void)DependencyGraph::build(catalog);
        } catch (const std::exception&) {
            rejected = true;
        }
        HH_REQUIRE(rejected);
        std::error_code ec;
        fs::remove_all(root, ec);
    }
    {
        const auto root = makeRepo("cycle");
        addAsset(root, "asset.a", "a", "[\"asset.b\"]");
        addAsset(root, "asset.b", "b", "[\"asset.a\"]");
        const auto catalog = AssetCatalog::scan(root / "Art/Exports");
        bool rejected = false;
        try {
            (void)DependencyGraph::build(catalog);
        } catch (const std::exception&) {
            rejected = true;
        }
        HH_REQUIRE(rejected);
        std::error_code ec;
        fs::remove_all(root, ec);
    }
}

HH_TEST("metadata fingerprints and Hasset round trips remain deterministic under stress") {
    const auto root = makeRepo("identity");
    addAsset(root, "asset.base", "base");
    addAsset(root, "asset.child", "child", "[\"asset.base\"]");
    const auto catalog = AssetCatalog::scan(root / "Art/Exports");
    const auto graph = DependencyGraph::build(catalog);
    const FingerprintSettings settings{"importer-1", "cooker-1", "none", "windows-x64"};
    const auto& record = catalog.by_id("asset.child");
    const auto expectedFingerprint = compute_fingerprint(record, catalog, graph, settings);
    const auto expectedMetadata = canonicalize_metadata(record.metadata);
    HH_REQUIRE(!expectedFingerprint.empty());
    HH_REQUIRE(!expectedMetadata.empty());

    HassetDocument document;
    document.type = AssetType::StaticMesh;
    document.asset_id = "asset.child";
    document.fingerprint = expectedFingerprint;
    document.dependencies = {"asset.base"};
    document.source_path = record.source_path.generic_string();
    document.sidecar_path = record.sidecar_path.generic_string();
    document.payload = {std::byte{0x01}, std::byte{0x02}, std::byte{0x03}, std::byte{0x04}};
    const auto encoded = serialize_hasset(document);

    const std::size_t iterations = std::max<std::size_t>(2'000, operationBudget() / 5);
    for (std::size_t i = 0; i < iterations; ++i) {
        HH_REQUIRE(compute_fingerprint(record, catalog, graph, settings) == expectedFingerprint);
        HH_REQUIRE(canonicalize_metadata(record.metadata) == expectedMetadata);
        const auto parsed = parse_hasset(encoded);
        HH_REQUIRE(parsed.asset_id == document.asset_id);
        HH_REQUIRE(parsed.fingerprint == document.fingerprint);
        HH_REQUIRE(parsed.dependencies == document.dependencies);
        HH_REQUIRE(parsed.payload == document.payload);
    }

    for (std::size_t cut = 0; cut < std::min<std::size_t>(encoded.size(), 64); ++cut) {
        std::vector<std::byte> truncated(encoded.begin(), encoded.begin() + cut);
        bool rejected = false;
        try {
            (void)parse_hasset(truncated);
        } catch (const std::exception&) {
            rejected = true;
        }
        HH_REQUIRE(rejected);
    }

    std::error_code ec;
    fs::remove_all(root, ec);
}

HH_TEST("cooker output is byte stable across repeated full cooks") {
    const auto root = makeRepo("cook");
    constexpr std::size_t kAssets = 64;
    for (std::size_t i = 0; i < kAssets; ++i) {
        const auto id = "cook." + std::to_string(i);
        addAsset(root, id, "cook_" + std::to_string(i),
                 i == 0 ? "[]" : dependencyJson("cook." + std::to_string(i - 1)));
    }
    const auto catalog = AssetCatalog::scan(root / "Art/Exports");
    const auto graph = DependencyGraph::build(catalog);
    const auto first = cook_all(catalog, graph, options(root));
    HH_REQUIRE(first.size() == kAssets);
    std::vector<std::vector<std::byte>> firstBytes;
    firstBytes.reserve(first.size());
    for (const auto& result : first) {
        HH_REQUIRE(result.cooked);
        HH_REQUIRE(fs::exists(result.output));
        firstBytes.push_back(readBytes(result.output));
    }

    for (int cycle = 0; cycle < 10; ++cycle) {
        const auto again = cook_all(catalog, graph, options(root));
        HH_REQUIRE(again.size() == kAssets);
        for (std::size_t i = 0; i < again.size(); ++i) {
            HH_REQUIRE(again[i].asset_id == first[i].asset_id);
            HH_REQUIRE(again[i].fingerprint == first[i].fingerprint);
            HH_REQUIRE(readBytes(again[i].output) == firstBytes[i]);
        }
    }

    const auto protectedOutput = first.back().output;
    const auto protectedBytes = readBytes(protectedOutput);
    fs::remove(catalog.by_id(first.back().asset_id).export_path);
    bool rejected = false;
    try {
        (void)cook_one(catalog, graph, first.back().asset_id, options(root));
    } catch (const std::exception&) {
        rejected = true;
    }
    HH_REQUIRE(rejected);
    HH_REQUIRE(readBytes(protectedOutput) == protectedBytes);
    HH_REQUIRE(!fs::exists(protectedOutput.string() + ".tmp"));

    std::error_code ec;
    fs::remove_all(root, ec);
}
