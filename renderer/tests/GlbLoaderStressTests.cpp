#include "TestFramework.h"
#include "hh/renderer/GlbLoader.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

std::size_t mutationBudget() {
    const char* scale = std::getenv("HH_STRESS_SCALE");
    const std::string_view value = scale ? std::string_view{scale} : std::string_view{"pr"};
    if (value == "pr") return 10'000;
    if (value == "extended") return 100'000;
    if (value == "exhaustive") return 1'000'000;
    throw std::runtime_error("HH_STRESS_SCALE must be pr, extended, or exhaustive");
}

void appendU32(std::vector<std::byte>& bytes, std::uint32_t value) {
    for (unsigned shift = 0; shift < 32u; shift += 8u)
        bytes.push_back(static_cast<std::byte>((value >> shift) & 0xffu));
}

void writeU32(std::vector<std::byte>& bytes, std::size_t offset, std::uint32_t value) {
    if (offset + 4 > bytes.size())
        throw std::runtime_error("stress mutation offset out of range");
    for (unsigned shift = 0; shift < 32u; shift += 8u)
        bytes[offset + shift / 8u] = static_cast<std::byte>((value >> shift) & 0xffu);
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

struct GlbFixture {
    std::vector<std::byte> bytes;
    std::uint32_t jsonLength{};
    std::size_t binHeaderOffset{};
};

GlbFixture makeGlb(bool malformedAccessor = false) {
    std::vector<std::byte> binary;
    appendVec3(binary, 1.0f, 2.0f, 3.0f);
    appendVec3(binary, 0.0f, 0.0f, 0.0f);
    appendVec3(binary, 0.0f, 1.0f, 0.0f);
    const std::size_t normalOffset = binary.size();
    appendVec3(binary, 0.0f, 0.0f, 1.0f);
    appendVec3(binary, 0.0f, 0.0f, 1.0f);
    appendVec3(binary, 0.0f, 0.0f, 1.0f);
    const std::size_t indexOffset = binary.size();
    const std::array<std::uint16_t, 3> indices{0u, 1u, 2u};
    for (const auto index : indices)
        appendValue(binary, index);
    while ((binary.size() % 4u) != 0u)
        binary.push_back(std::byte{0});

    const std::size_t positionCount = malformedAccessor ? 99u : 3u;
    std::string json =
        "{\"asset\":{\"version\":\"2.0\"},"
        "\"buffers\":[{\"byteLength\":" + std::to_string(binary.size()) + "}],"
        "\"bufferViews\":["
        "{\"buffer\":0,\"byteOffset\":0,\"byteLength\":" + std::to_string(normalOffset) + "},"
        "{\"buffer\":0,\"byteOffset\":" + std::to_string(normalOffset) + ",\"byteLength\":" + std::to_string(indexOffset - normalOffset) + "},"
        "{\"buffer\":0,\"byteOffset\":" + std::to_string(indexOffset) + ",\"byteLength\":6}],"
        "\"accessors\":["
        "{\"bufferView\":0,\"componentType\":5126,\"count\":" + std::to_string(positionCount) + ",\"type\":\"VEC3\"},"
        "{\"bufferView\":1,\"componentType\":5126,\"count\":3,\"type\":\"VEC3\"},"
        "{\"bufferView\":2,\"componentType\":5123,\"count\":3,\"type\":\"SCALAR\"}],"
        "\"meshes\":[{\"primitives\":[{\"attributes\":{\"POSITION\":0,\"NORMAL\":1},\"indices\":2,\"mode\":4}]}],"
        "\"nodes\":[{\"mesh\":0}],\"scenes\":[{\"nodes\":[0]}],\"scene\":0}";
    while ((json.size() % 4u) != 0u)
        json.push_back(' ');

    const std::uint32_t jsonLength = static_cast<std::uint32_t>(json.size());
    const std::uint32_t binaryLength = static_cast<std::uint32_t>(binary.size());
    const std::uint32_t totalLength = 12u + 8u + jsonLength + 8u + binaryLength;
    GlbFixture fixture;
    fixture.bytes.reserve(totalLength);
    appendU32(fixture.bytes, 0x46546c67u);
    appendU32(fixture.bytes, 2u);
    appendU32(fixture.bytes, totalLength);
    appendU32(fixture.bytes, jsonLength);
    appendU32(fixture.bytes, 0x4e4f534au);
    for (const char c : json)
        fixture.bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(c)));
    fixture.binHeaderOffset = fixture.bytes.size();
    appendU32(fixture.bytes, binaryLength);
    appendU32(fixture.bytes, 0x004e4942u);
    fixture.bytes.insert(fixture.bytes.end(), binary.begin(), binary.end());
    fixture.jsonLength = jsonLength;
    return fixture;
}

bool rejects(const std::vector<std::byte>& bytes) {
    try {
        (void)hh::renderer::loadGlbMesh(bytes);
        return false;
    } catch (const std::runtime_error&) {
        return true;
    }
}

std::vector<std::byte> mutation(std::size_t index) {
    const auto valid = makeGlb();
    auto bytes = valid.bytes;
    switch (index % 8) {
    case 0:
        bytes.resize(index % 12);
        break;
    case 1:
        writeU32(bytes, 0, 0xDEADBEEFu);
        break;
    case 2:
        writeU32(bytes, 4, 1u);
        break;
    case 3:
        writeU32(bytes, 8, static_cast<std::uint32_t>(bytes.size() + 4096u));
        break;
    case 4:
        writeU32(bytes, 12, valid.jsonLength + 4096u);
        break;
    case 5:
        writeU32(bytes, 16, 0x004e4942u);
        break;
    case 6:
        bytes.pop_back();
        break;
    case 7:
        writeU32(bytes, valid.binHeaderOffset, 0xfffffff0u);
        break;
    }
    return bytes;
}

}  // namespace

TEST_CASE("GLB loader repeatedly accepts bounded valid geometry") {
    const auto fixture = makeGlb();
    const std::size_t iterations = std::max<std::size_t>(1'000, mutationBudget() / 10);
    for (std::size_t i = 0; i < iterations; ++i) {
        const auto mesh = hh::renderer::loadGlbMesh(fixture.bytes);
        EXPECT_EQ(mesh.primitives.size(), 1u);
        EXPECT_EQ(mesh.primitives.front().vertices.size(), 3u);
        EXPECT_EQ(mesh.primitives.front().indices.size(), 3u);
    }
}

TEST_CASE("GLB loader cleanly rejects deterministic malformed corpus") {
    const std::size_t iterations = mutationBudget();
    for (std::size_t i = 0; i < iterations; ++i)
        EXPECT_TRUE(rejects(mutation(i)));
}

TEST_CASE("GLB loader rejects accessor bounds pressure repeatedly") {
    const auto malformed = makeGlb(true);
    const std::size_t iterations = std::max<std::size_t>(1'000, mutationBudget() / 10);
    for (std::size_t i = 0; i < iterations; ++i)
        EXPECT_TRUE(rejects(malformed.bytes));
}
