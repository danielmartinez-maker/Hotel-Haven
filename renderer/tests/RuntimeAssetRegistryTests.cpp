#include "TestFramework.h"

#include "hh/assets/Hasset.h"
#include "hh/renderer/RuntimeAssetRegistry.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void appendU32(std::vector<std::byte>& bytes, std::uint32_t value) {
    for (unsigned shift = 0; shift < 32u; shift += 8u) {
        bytes.push_back(static_cast<std::byte>((value >> shift) & 0xffu));
    }
}

template <typename T>
void appendValue(std::vector<std::byte>& bytes, const T& value) {
    const auto* first = reinterpret_cast<const std::byte*>(&value);
    bytes.insert(bytes.end(), first, first + sizeof(T));
}

std::vector<std::byte> makeTriangleGlb() {
    std::vector<std::byte> binary;
    const std::array<float, 9> positions{
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
    };
    for (const float value : positions) {
        appendValue(binary, value);
    }
    const std::size_t indexOffset = binary.size();
    const std::array<std::uint16_t, 3> indices{0u, 1u, 2u};
    for (const auto value : indices) {
        appendValue(binary, value);
    }
    while ((binary.size() % 4u) != 0u) {
        binary.push_back(std::byte{0});
    }

    std::string json =
        "{"
        "\"asset\":{\"version\":\"2.0\"},"
        "\"buffers\":[{\"byteLength\":" + std::to_string(binary.size()) + "}],"
        "\"bufferViews\":["
          "{\"buffer\":0,\"byteOffset\":0,\"byteLength\":36},"
          "{\"buffer\":0,\"byteOffset\":" + std::to_string(indexOffset) + ",\"byteLength\":6}"
        "],"
        "\"accessors\":["
          "{\"bufferView\":0,\"componentType\":5126,\"count\":3,\"type\":\"VEC3\"},"
          "{\"bufferView\":1,\"componentType\":5123,\"count\":3,\"type\":\"SCALAR\"}"
        "],"
        "\"meshes\":[{\"primitives\":[{\"attributes\":{\"POSITION\":0},\"indices\":1}]}],"
        "\"nodes\":[{\"mesh\":0}],"
        "\"scenes\":[{\"nodes\":[0]}],\"scene\":0"
        "}";
    while ((json.size() % 4u) != 0u) {
        json.push_back(' ');
    }

    const std::uint32_t jsonLength = static_cast<std::uint32_t>(json.size());
    const std::uint32_t binaryLength = static_cast<std::uint32_t>(binary.size());
    std::vector<std::byte> glb;
    appendU32(glb, 0x46546c67u);
    appendU32(glb, 2u);
    appendU32(glb, 12u + 8u + jsonLength + 8u + binaryLength);
    appendU32(glb, jsonLength);
    appendU32(glb, 0x4e4f534au);
    for (const char c : json) {
        glb.push_back(static_cast<std::byte>(static_cast<unsigned char>(c)));
    }
    appendU32(glb, binaryLength);
    appendU32(glb, 0x004e4942u);
    glb.insert(glb.end(), binary.begin(), binary.end());
    return glb;
}

std::vector<std::byte> makeHasset(std::string id, hh::assets::AssetType type) {
    hh::assets::HassetDocument document;
    document.type = type;
    document.asset_id = std::move(id);
    document.fingerprint = "test-fingerprint";
    document.source_path = "Art/Exports/test.glb";
    document.sidecar_path = "Art/Exports/test.asset.json";
    document.payload = makeTriangleGlb();
    return hh::assets::serialize_hasset(document);
}

}  // namespace

TEST_CASE("runtime asset registry resolves cooked static meshes to stable handles") {
    hh::renderer::RuntimeAssetRegistry registry;
    const auto handle = registry.addHasset(makeHasset("HH_A001", hh::assets::AssetType::StaticMesh));

    EXPECT_EQ(registry.size(), 1u);
    EXPECT_EQ(handle.value, 0u);
    EXPECT_EQ(registry.resolve("HH_A001").value, handle.value);
    EXPECT_EQ(registry.asset(handle).assetId, std::string("HH_A001"));
    EXPECT_EQ(registry.asset(handle).mesh.primitives.size(), 1u);
}

TEST_CASE("runtime asset registry rejects duplicate asset IDs") {
    hh::renderer::RuntimeAssetRegistry registry;
    (void)registry.addHasset(makeHasset("HH_A001", hh::assets::AssetType::StaticMesh));
    bool threw = false;
    try {
        (void)registry.addHasset(makeHasset("HH_A001", hh::assets::AssetType::StaticMesh));
    } catch (const std::runtime_error&) {
        threw = true;
    }
    EXPECT_TRUE(threw);
}

TEST_CASE("runtime asset registry rejects cooked non-static assets") {
    hh::renderer::RuntimeAssetRegistry registry;
    bool threw = false;
    try {
        (void)registry.addHasset(makeHasset("HH_A001", hh::assets::AssetType::Material));
    } catch (const std::runtime_error&) {
        threw = true;
    }
    EXPECT_TRUE(threw);
}
