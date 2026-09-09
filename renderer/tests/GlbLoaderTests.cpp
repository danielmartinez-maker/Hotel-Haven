#include "TestFramework.h"

#include "hh/renderer/GlbLoader.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
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

void appendVec3(std::vector<std::byte>& bytes, float x, float y, float z) {
    appendValue(bytes, x);
    appendValue(bytes, y);
    appendValue(bytes, z);
}

std::vector<std::byte> makeGlb(bool malformedAccessor = false) {
    std::vector<std::byte> binary;

    // Positions: one triangle in glTF right-handed, Y-up coordinates.
    appendVec3(binary, 1.0f, 2.0f, 3.0f);
    appendVec3(binary, 0.0f, 0.0f, 0.0f);
    appendVec3(binary, 0.0f, 1.0f, 0.0f);
    const std::size_t normalOffset = binary.size();

    appendVec3(binary, 0.0f, 0.0f, 1.0f);
    appendVec3(binary, 0.0f, 0.0f, 1.0f);
    appendVec3(binary, 0.0f, 0.0f, 1.0f);
    const std::size_t indexOffset = binary.size();

    const std::array<std::uint16_t, 3> indices{0u, 1u, 2u};
    for (const auto index : indices) {
        appendValue(binary, index);
    }
    while ((binary.size() % 4u) != 0u) {
        binary.push_back(std::byte{0});
    }

    const std::size_t positionBytes = normalOffset;
    const std::size_t normalBytes = indexOffset - normalOffset;
    const std::size_t indexBytes = indices.size() * sizeof(std::uint16_t);
    const std::size_t positionCount = malformedAccessor ? 99u : 3u;

    std::string json =
        "{"
        "\"asset\":{\"version\":\"2.0\"},"
        "\"buffers\":[{\"byteLength\":" + std::to_string(binary.size()) + "}],"
        "\"bufferViews\":["
          "{\"buffer\":0,\"byteOffset\":0,\"byteLength\":" + std::to_string(positionBytes) + "},"
          "{\"buffer\":0,\"byteOffset\":" + std::to_string(normalOffset) + ",\"byteLength\":" + std::to_string(normalBytes) + "},"
          "{\"buffer\":0,\"byteOffset\":" + std::to_string(indexOffset) + ",\"byteLength\":" + std::to_string(indexBytes) + "}"
        "],"
        "\"accessors\":["
          "{\"bufferView\":0,\"componentType\":5126,\"count\":" + std::to_string(positionCount) + ",\"type\":\"VEC3\"},"
          "{\"bufferView\":1,\"componentType\":5126,\"count\":3,\"type\":\"VEC3\"},"
          "{\"bufferView\":2,\"componentType\":5123,\"count\":3,\"type\":\"SCALAR\"}"
        "],"
        "\"materials\":[{"
          "\"pbrMetallicRoughness\":{"
            "\"baseColorFactor\":[0.2,0.3,0.4,0.5],"
            "\"metallicFactor\":0.7,"
            "\"roughnessFactor\":0.25"
          "},"
          "\"alphaMode\":\"BLEND\""
        "}],"
        "\"meshes\":[{\"primitives\":[{"
          "\"attributes\":{\"POSITION\":0,\"NORMAL\":1},"
          "\"indices\":2,\"material\":0,\"mode\":4"
        "}]}],"
        "\"nodes\":[{\"mesh\":0,\"translation\":[2.0,0.0,1.0]}],"
        "\"scenes\":[{\"nodes\":[0]}],\"scene\":0"
        "}";

    while ((json.size() % 4u) != 0u) {
        json.push_back(' ');
    }

    const std::uint32_t jsonLength = static_cast<std::uint32_t>(json.size());
    const std::uint32_t binaryLength = static_cast<std::uint32_t>(binary.size());
    const std::uint32_t totalLength = 12u + 8u + jsonLength + 8u + binaryLength;

    std::vector<std::byte> glb;
    glb.reserve(totalLength);
    appendU32(glb, 0x46546c67u);
    appendU32(glb, 2u);
    appendU32(glb, totalLength);
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

}  // namespace

TEST_CASE("GLB loader converts handedness, node transform, normals, winding, and material alpha") {
    const auto mesh = hh::renderer::loadGlbMesh(makeGlb());

    EXPECT_EQ(mesh.primitives.size(), 1u);
    EXPECT_EQ(mesh.materials.size(), 1u);
    const auto& primitive = mesh.primitives.front();
    EXPECT_EQ(primitive.vertices.size(), 3u);
    EXPECT_EQ(primitive.indices.size(), 3u);

    // Node translation is applied in glTF coordinates before RH -> LH conversion.
    EXPECT_NEAR(primitive.vertices[0].position.x, 3.0f, 0.0001f);
    EXPECT_NEAR(primitive.vertices[0].position.y, 2.0f, 0.0001f);
    EXPECT_NEAR(primitive.vertices[0].position.z, -4.0f, 0.0001f);
    EXPECT_NEAR(primitive.vertices[0].normal.x, 0.0f, 0.0001f);
    EXPECT_NEAR(primitive.vertices[0].normal.y, 0.0f, 0.0001f);
    EXPECT_NEAR(primitive.vertices[0].normal.z, -1.0f, 0.0001f);

    // Mirroring Z changes handedness, so triangle winding must be reversed.
    EXPECT_EQ(primitive.indices[0], 0u);
    EXPECT_EQ(primitive.indices[1], 2u);
    EXPECT_EQ(primitive.indices[2], 1u);

    const auto& material = mesh.materials.front();
    EXPECT_NEAR(material.baseColor.r, 0.2f, 0.0001f);
    EXPECT_NEAR(material.baseColor.g, 0.3f, 0.0001f);
    EXPECT_NEAR(material.baseColor.b, 0.4f, 0.0001f);
    EXPECT_NEAR(material.baseColor.a, 0.5f, 0.0001f);
    EXPECT_NEAR(material.metallic, 0.7f, 0.0001f);
    EXPECT_NEAR(material.roughness, 0.25f, 0.0001f);
    EXPECT_TRUE(material.translucent);

    EXPECT_NEAR(mesh.bounds.min.x, 2.0f, 0.0001f);
    EXPECT_NEAR(mesh.bounds.max.x, 3.0f, 0.0001f);
    EXPECT_NEAR(mesh.bounds.min.z, -4.0f, 0.0001f);
    EXPECT_NEAR(mesh.bounds.max.z, -1.0f, 0.0001f);
}

TEST_CASE("GLB loader rejects accessors that overrun their buffer view") {
    bool threw = false;
    try {
        (void)hh::renderer::loadGlbMesh(makeGlb(true));
    } catch (const std::runtime_error&) {
        threw = true;
    }
    EXPECT_TRUE(threw);
}
