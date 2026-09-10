#include "TestFramework.h"
#include "hh/assets/Hasset.h"
#include "hh/renderer/RuntimeAssetRegistry.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
using hh::assets::AssetType;

std::size_t operationBudget() {
    const char* scale = std::getenv("HH_STRESS_SCALE");
    const std::string_view value = scale ? std::string_view{scale} : std::string_view{"pr"};
    if (value == "pr") return 100'000;
    if (value == "extended") return 1'000'000;
    if (value == "exhaustive") return 5'000'000;
    throw std::runtime_error("HH_STRESS_SCALE must be pr, extended, or exhaustive");
}

void appendU32(std::vector<std::byte>& bytes, std::uint32_t value) {
    for (unsigned shift = 0; shift < 32u; shift += 8u)
        bytes.push_back(static_cast<std::byte>((value >> shift) & 0xffu));
}

template <typename T>
void appendValue(std::vector<std::byte>& bytes, const T& value) {
    const auto* first = reinterpret_cast<const std::byte*>(&value);
    bytes.insert(bytes.end(), first, first + sizeof(T));
}

std::vector<std::byte> triangleGlb() {
    std::vector<std::byte> binary;
    const std::array<float, 9> positions{0.0f, 0.0f, 0.0f,
                                         1.0f, 0.0f, 0.0f,
                                         0.0f, 1.0f, 0.0f};
    for (float value : positions)
        appendValue(binary, value);
    const std::size_t indexOffset = binary.size();
    const std::array<std::uint16_t, 3> indices{0u, 1u, 2u};
    for (auto value : indices)
        appendValue(binary, value);
    while (binary.size() % 4u != 0u)
        binary.push_back(std::byte{0});

    std::string json =
        "{\"asset\":{\"version\":\"2.0\"},"
        "\"buffers\":[{\"byteLength\":" + std::to_string(binary.size()) + "}],"
        "\"bufferViews\":[{\"buffer\":0,\"byteOffset\":0,\"byteLength\":36},"
        "{\"buffer\":0,\"byteOffset\":" + std::to_string(indexOffset) + ",\"byteLength\":6}],"
        "\"accessors\":[{\"bufferView\":0,\"componentType\":5126,\"count\":3,\"type\":\"VEC3\"},"
        "{\"bufferView\":1,\"componentType\":5123,\"count\":3,\"type\":\"SCALAR\"}],"
        "\"meshes\":[{\"primitives\":[{\"attributes\":{\"POSITION\":0},\"indices\":1}]}],"
        "\"nodes\":[{\"mesh\":0}],\"scenes\":[{\"nodes\":[0]}],\"scene\":0}";
    while (json.size() % 4u != 0u)
        json.push_back(' ');

    const auto jsonLength = static_cast<std::uint32_t>(json.size());
    const auto binaryLength = static_cast<std::uint32_t>(binary.size());
    std::vector<std::byte> glb;
    appendU32(glb, 0x46546c67u);
    appendU32(glb, 2u);
    appendU32(glb, 12u + 8u + jsonLength + 8u + binaryLength);
    appendU32(glb, jsonLength);
    appendU32(glb, 0x4e4f534au);
    for (char c : json)
        glb.push_back(static_cast<std::byte>(static_cast<unsigned char>(c)));
    appendU32(glb, binaryLength);
    appendU32(glb, 0x004e4942u);
    glb.insert(glb.end(), binary.begin(), binary.end());
    return glb;
}

std::vector<std::byte> makeHasset(const std::string& id, AssetType type = AssetType::StaticMesh) {
    hh::assets::HassetDocument document;
    document.type = type;
    document.asset_id = id;
    document.fingerprint = "stress-fingerprint-" + id;
    document.source_path = "Art/Exports/" + id + ".glb";
    document.sidecar_path = "Art/Exports/" + id + ".asset.json";
    document.payload = triangleGlb();
    return hh::assets::serialize_hasset(document);
}

bool throwsUnknown(const hh::renderer::RuntimeAssetRegistry& registry,
                   std::string_view id) {
    try {
        (void)registry.resolve(id);
        return false;
    } catch (const std::runtime_error&) {
        return true;
    }
}

std::filesystem::path tempRoot() {
    const auto root = std::filesystem::temp_directory_path() /
                      "hotel_haven_runtime_registry_stress";
    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    std::filesystem::create_directories(root);
    return root;
}

void writeBytes(const std::filesystem::path& path,
                const std::vector<std::byte>& bytes) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output)
        throw std::runtime_error("could not create registry stress fixture");
    output.write(reinterpret_cast<const char*>(bytes.data()),
                 static_cast<std::streamsize>(bytes.size()));
    if (!output)
        throw std::runtime_error("could not write registry stress fixture");
}
}  // namespace

TEST_CASE("runtime asset registry preserves stable identity across 100k lookups") {
    hh::renderer::RuntimeAssetRegistry registry;
    constexpr std::size_t kAssets = 256;
    std::array<hh::renderer::AssetHandle, kAssets> handles{};
    for (std::size_t i = 0; i < kAssets; ++i) {
        const std::string id = "HH_STRESS_" + std::to_string(i);
        handles[i] = registry.addHasset(makeHasset(id));
        EXPECT_EQ(handles[i].value, static_cast<std::uint32_t>(i));
        EXPECT_EQ(registry.asset(handles[i]).assetId, id);
    }
    EXPECT_EQ(registry.size(), kAssets);

    const std::size_t operations = operationBudget();
    for (std::size_t i = 0; i < operations; ++i) {
        const std::size_t index = (i * 2654435761ULL) % kAssets;
        const std::string id = "HH_STRESS_" + std::to_string(index);
        const auto resolved = registry.resolve(id);
        EXPECT_EQ(resolved.value, handles[index].value);
        EXPECT_EQ(registry.asset(resolved).assetId, id);
        if ((i % 4096u) == 0u) {
            const auto before = registry.size();
            bool duplicateRejected = false;
            try {
                (void)registry.addHasset(makeHasset(id));
            } catch (const std::runtime_error&) {
                duplicateRejected = true;
            }
            EXPECT_TRUE(duplicateRejected);
            EXPECT_EQ(registry.size(), before);
            EXPECT_TRUE(throwsUnknown(registry, "HH_DOES_NOT_EXIST"));
        }
    }
    EXPECT_EQ(registry.size(), kAssets);
}

TEST_CASE("runtime registry directory reload is atomic under corrupt input") {
    const auto root = tempRoot();
    constexpr std::size_t kFiles = 32;
    for (std::size_t i = 0; i < kFiles; ++i)
        writeBytes(root / ("asset_" + std::to_string(i) + ".hasset"),
                   makeHasset("HH_DIR_" + std::to_string(i)));

    hh::renderer::RuntimeAssetRegistry registry;
    for (int cycle = 0; cycle < 50; ++cycle) {
        registry.loadDirectory(root);
        EXPECT_EQ(registry.size(), kFiles);
        for (std::size_t i = 0; i < kFiles; ++i) {
            const auto id = "HH_DIR_" + std::to_string(i);
            EXPECT_EQ(registry.asset(registry.resolve(id)).assetId, id);
        }
    }

    const auto stableSize = registry.size();
    const auto stableHandle = registry.resolve("HH_DIR_0").value;
    writeBytes(root / "corrupt.hasset", {std::byte{0x01}, std::byte{0x02}});
    bool rejected = false;
    try {
        registry.loadDirectory(root);
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    EXPECT_TRUE(rejected);
    EXPECT_EQ(registry.size(), stableSize);
    EXPECT_EQ(registry.resolve("HH_DIR_0").value, stableHandle);

    std::error_code ec;
    std::filesystem::remove_all(root, ec);
}

TEST_CASE("runtime registry rejects non mesh payload without consuming handle") {
    hh::renderer::RuntimeAssetRegistry registry;
    bool rejected = false;
    try {
        (void)registry.addHasset(makeHasset("HH_BAD", AssetType::Material));
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    EXPECT_TRUE(rejected);
    EXPECT_EQ(registry.size(), 0u);
    const auto handle = registry.addHasset(makeHasset("HH_GOOD"));
    EXPECT_EQ(handle.value, 0u);
}
