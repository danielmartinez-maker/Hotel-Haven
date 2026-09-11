#include "hh/renderer/GlbLoader.h"

#include "hh/assets/Json.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <functional>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace hh::renderer {
namespace {

constexpr std::uint32_t kGlbMagic = 0x46546c67u;
constexpr std::uint32_t kGlbVersion = 2u;
constexpr std::uint32_t kJsonChunk = 0x4e4f534au;
constexpr std::uint32_t kBinChunk = 0x004e4942u;
constexpr std::uint32_t kFloatComponent = 5126u;
constexpr std::uint32_t kUnsignedByteComponent = 5121u;
constexpr std::uint32_t kUnsignedShortComponent = 5123u;
constexpr std::uint32_t kUnsignedIntComponent = 5125u;
constexpr std::uint32_t kTrianglesMode = 4u;
constexpr float kEpsilon = 1.0e-8f;

using JsonValue = hh::assets::JsonValue;

struct BufferViewInfo {
    std::size_t byteOffset{};
    std::size_t byteLength{};
    std::size_t byteStride{};
};

struct AccessorInfo {
    std::size_t bufferView{};
    std::size_t byteOffset{};
    std::size_t count{};
    std::uint32_t componentType{};
    std::string type;
};

struct Mat4 {
    std::array<float, 16> values{};
};

std::uint32_t readU32(std::span<const std::byte> bytes, std::size_t offset) {
    if (offset > bytes.size() || bytes.size() - offset < 4u) {
        throw std::runtime_error("truncated GLB integer");
    }
    std::uint32_t value = 0u;
    for (unsigned shift = 0u; shift < 32u; shift += 8u) {
        value |= std::to_integer<std::uint32_t>(bytes[offset++]) << shift;
    }
    return value;
}

std::size_t asIndex(const JsonValue& value, std::string_view label) {
    const double number = value.as_number();
    if (number < 0.0 || std::floor(number) != number ||
        number > static_cast<double>(std::numeric_limits<std::size_t>::max())) {
        throw std::runtime_error("invalid non-negative integer for " + std::string(label));
    }
    return static_cast<std::size_t>(number);
}

std::uint32_t asU32(const JsonValue& value, std::string_view label) {
    const std::size_t index = asIndex(value, label);
    if (index > std::numeric_limits<std::uint32_t>::max()) {
        throw std::runtime_error("integer is too large for " + std::string(label));
    }
    return static_cast<std::uint32_t>(index);
}

float asFloat(const JsonValue& value, std::string_view label) {
    const double number = value.as_number();
    if (number < -static_cast<double>(std::numeric_limits<float>::max()) ||
        number > static_cast<double>(std::numeric_limits<float>::max())) {
        throw std::runtime_error("number is out of float range for " + std::string(label));
    }
    return static_cast<float>(number);
}

std::vector<float> floatArray(const JsonValue& value, std::size_t expected, std::string_view label) {
    const auto& array = value.as_array();
    if (array.size() != expected) {
        throw std::runtime_error(std::string(label) + " has unexpected element count");
    }
    std::vector<float> result;
    result.reserve(expected);
    for (const auto& item : array) {
        result.push_back(asFloat(item, label));
    }
    return result;
}

Mat4 identityMatrix() {
    Mat4 matrix;
    matrix.values[0] = 1.0f;
    matrix.values[5] = 1.0f;
    matrix.values[10] = 1.0f;
    matrix.values[15] = 1.0f;
    return matrix;
}

Mat4 multiply(const Mat4& lhs, const Mat4& rhs) {
    Mat4 result;
    for (std::size_t column = 0; column < 4u; ++column) {
        for (std::size_t row = 0; row < 4u; ++row) {
            float value = 0.0f;
            for (std::size_t k = 0; k < 4u; ++k) {
                value += lhs.values[k * 4u + row] * rhs.values[column * 4u + k];
            }
            result.values[column * 4u + row] = value;
        }
    }
    return result;
}

Mat4 translationMatrix(float x, float y, float z) {
    Mat4 matrix = identityMatrix();
    matrix.values[12] = x;
    matrix.values[13] = y;
    matrix.values[14] = z;
    return matrix;
}

Mat4 scaleMatrix(float x, float y, float z) {
    Mat4 matrix{};
    matrix.values[0] = x;
    matrix.values[5] = y;
    matrix.values[10] = z;
    matrix.values[15] = 1.0f;
    return matrix;
}

Mat4 rotationMatrix(float x, float y, float z, float w) {
    const float length = std::sqrt(x * x + y * y + z * z + w * w);
    if (length < kEpsilon) {
        throw std::runtime_error("GLB node rotation quaternion has zero length");
    }
    x /= length;
    y /= length;
    z /= length;
    w /= length;

    const float xx = x * x;
    const float yy = y * y;
    const float zz = z * z;
    const float xy = x * y;
    const float xz = x * z;
    const float yz = y * z;
    const float xw = x * w;
    const float yw = y * w;
    const float zw = z * w;

    Mat4 matrix = identityMatrix();
    matrix.values[0] = 1.0f - 2.0f * (yy + zz);
    matrix.values[1] = 2.0f * (xy + zw);
    matrix.values[2] = 2.0f * (xz - yw);
    matrix.values[4] = 2.0f * (xy - zw);
    matrix.values[5] = 1.0f - 2.0f * (xx + zz);
    matrix.values[6] = 2.0f * (yz + xw);
    matrix.values[8] = 2.0f * (xz + yw);
    matrix.values[9] = 2.0f * (yz - xw);
    matrix.values[10] = 1.0f - 2.0f * (xx + yy);
    return matrix;
}

Mat4 nodeLocalMatrix(const JsonValue& node) {
    if (const JsonValue* matrixValue = node.find("matrix")) {
        const auto values = floatArray(*matrixValue, 16u, "node.matrix");
        Mat4 matrix;
        std::copy(values.begin(), values.end(), matrix.values.begin());
        return matrix;
    }

    std::array<float, 3> translation{0.0f, 0.0f, 0.0f};
    std::array<float, 4> rotation{0.0f, 0.0f, 0.0f, 1.0f};
    std::array<float, 3> scale{1.0f, 1.0f, 1.0f};

    if (const JsonValue* value = node.find("translation")) {
        const auto values = floatArray(*value, 3u, "node.translation");
        std::copy(values.begin(), values.end(), translation.begin());
    }
    if (const JsonValue* value = node.find("rotation")) {
        const auto values = floatArray(*value, 4u, "node.rotation");
        std::copy(values.begin(), values.end(), rotation.begin());
    }
    if (const JsonValue* value = node.find("scale")) {
        const auto values = floatArray(*value, 3u, "node.scale");
        std::copy(values.begin(), values.end(), scale.begin());
    }

    return multiply(
        multiply(
            translationMatrix(translation[0], translation[1], translation[2]),
            rotationMatrix(rotation[0], rotation[1], rotation[2], rotation[3])),
        scaleMatrix(scale[0], scale[1], scale[2]));
}

Vec3 transformPoint(const Mat4& matrix, Vec3 point) {
    return {
        matrix.values[0] * point.x + matrix.values[4] * point.y + matrix.values[8] * point.z + matrix.values[12],
        matrix.values[1] * point.x + matrix.values[5] * point.y + matrix.values[9] * point.z + matrix.values[13],
        matrix.values[2] * point.x + matrix.values[6] * point.y + matrix.values[10] * point.z + matrix.values[14],
    };
}

Vec3 assetToRendererSpace(Vec3 value) {
    return {value.x, value.z, value.y};
}

Vec3 normalize(Vec3 value) {
    const float length = std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
    if (length < kEpsilon) {
        return {0.0f, 1.0f, 0.0f};
    }
    return {value.x / length, value.y / length, value.z / length};
}

Vec3 transformNormal(const Mat4& matrix, Vec3 normal) {
    const float a00 = matrix.values[0];
    const float a01 = matrix.values[4];
    const float a02 = matrix.values[8];
    const float a10 = matrix.values[1];
    const float a11 = matrix.values[5];
    const float a12 = matrix.values[9];
    const float a20 = matrix.values[2];
    const float a21 = matrix.values[6];
    const float a22 = matrix.values[10];

    const float c00 = a11 * a22 - a12 * a21;
    const float c01 = a12 * a20 - a10 * a22;
    const float c02 = a10 * a21 - a11 * a20;
    const float c10 = a02 * a21 - a01 * a22;
    const float c11 = a00 * a22 - a02 * a20;
    const float c12 = a01 * a20 - a00 * a21;
    const float c20 = a01 * a12 - a02 * a11;
    const float c21 = a02 * a10 - a00 * a12;
    const float c22 = a00 * a11 - a01 * a10;
    const float determinant = a00 * c00 + a01 * c01 + a02 * c02;
    if (std::fabs(determinant) < kEpsilon) {
        throw std::runtime_error("GLB node transform is singular");
    }

    const float inverseDeterminant = 1.0f / determinant;
    return normalize({
        (c00 * normal.x + c01 * normal.y + c02 * normal.z) * inverseDeterminant,
        (c10 * normal.x + c11 * normal.y + c12 * normal.z) * inverseDeterminant,
        (c20 * normal.x + c21 * normal.y + c22 * normal.z) * inverseDeterminant,
    });
}

Vec3 cross(Vec3 lhs, Vec3 rhs) {
    return {
        lhs.y * rhs.z - lhs.z * rhs.y,
        lhs.z * rhs.x - lhs.x * rhs.z,
        lhs.x * rhs.y - lhs.y * rhs.x,
    };
}

std::vector<BufferViewInfo> parseBufferViews(const JsonValue& root, std::size_t binarySize) {
    const JsonValue* viewsValue = root.find("bufferViews");
    if (viewsValue == nullptr) {
        return {};
    }

    std::vector<BufferViewInfo> views;
    for (const auto& value : viewsValue->as_array()) {
        if (asIndex(value.at("buffer"), "bufferView.buffer") != 0u) {
            throw std::runtime_error("GLB runtime supports only the embedded buffer 0");
        }
        BufferViewInfo view;
        if (const JsonValue* offset = value.find("byteOffset")) {
            view.byteOffset = asIndex(*offset, "bufferView.byteOffset");
        }
        view.byteLength = asIndex(value.at("byteLength"), "bufferView.byteLength");
        if (const JsonValue* stride = value.find("byteStride")) {
            view.byteStride = asIndex(*stride, "bufferView.byteStride");
        }
        if (view.byteOffset > binarySize || view.byteLength > binarySize - view.byteOffset) {
            throw std::runtime_error("GLB buffer view exceeds binary chunk");
        }
        views.push_back(view);
    }
    return views;
}

std::vector<AccessorInfo> parseAccessors(const JsonValue& root, std::size_t viewCount) {
    const JsonValue* accessorsValue = root.find("accessors");
    if (accessorsValue == nullptr) {
        return {};
    }

    std::vector<AccessorInfo> accessors;
    for (const auto& value : accessorsValue->as_array()) {
        if (value.find("sparse") != nullptr) {
            throw std::runtime_error("sparse GLB accessors are not supported by the runtime loader");
        }
        AccessorInfo accessor;
        accessor.bufferView = asIndex(value.at("bufferView"), "accessor.bufferView");
        if (accessor.bufferView >= viewCount) {
            throw std::runtime_error("GLB accessor references an invalid buffer view");
        }
        if (const JsonValue* offset = value.find("byteOffset")) {
            accessor.byteOffset = asIndex(*offset, "accessor.byteOffset");
        }
        accessor.componentType = asU32(value.at("componentType"), "accessor.componentType");
        accessor.count = asIndex(value.at("count"), "accessor.count");
        accessor.type = value.at("type").as_string();
        accessors.push_back(std::move(accessor));
    }
    return accessors;
}

void validateAccessorRange(
    const AccessorInfo& accessor,
    const BufferViewInfo& view,
    std::size_t elementSize) {
    const std::size_t stride = view.byteStride == 0u ? elementSize : view.byteStride;
    if (stride < elementSize) {
        throw std::runtime_error("GLB accessor byte stride is smaller than its element size");
    }
    if (accessor.byteOffset > view.byteLength) {
        throw std::runtime_error("GLB accessor starts outside its buffer view");
    }
    if (accessor.count == 0u) {
        return;
    }
    const std::size_t lastIndex = accessor.count - 1u;
    if (lastIndex > (std::numeric_limits<std::size_t>::max() - accessor.byteOffset - elementSize) / stride) {
        throw std::runtime_error("GLB accessor byte range overflow");
    }
    const std::size_t required = accessor.byteOffset + lastIndex * stride + elementSize;
    if (required > view.byteLength) {
        throw std::runtime_error("GLB accessor exceeds its buffer view");
    }
}

float readFloat(std::span<const std::byte> binary, std::size_t offset) {
    if (offset > binary.size() || binary.size() - offset < sizeof(float)) {
        throw std::runtime_error("truncated GLB float");
    }
    float value = 0.0f;
    std::memcpy(&value, binary.data() + offset, sizeof(float));
    if (!std::isfinite(value)) {
        throw std::runtime_error("non-finite GLB vertex component");
    }
    return value;
}

std::vector<Vec3> readVec3Accessor(
    std::size_t accessorIndex,
    const std::vector<AccessorInfo>& accessors,
    const std::vector<BufferViewInfo>& views,
    std::span<const std::byte> binary) {
    if (accessorIndex >= accessors.size()) {
        throw std::runtime_error("GLB primitive references an invalid accessor");
    }
    const auto& accessor = accessors[accessorIndex];
    if (accessor.componentType != kFloatComponent || accessor.type != "VEC3") {
        throw std::runtime_error("GLB POSITION/NORMAL accessor must be float VEC3");
    }
    const auto& view = views[accessor.bufferView];
    constexpr std::size_t elementSize = 3u * sizeof(float);
    validateAccessorRange(accessor, view, elementSize);
    const std::size_t stride = view.byteStride == 0u ? elementSize : view.byteStride;

    std::vector<Vec3> values;
    values.reserve(accessor.count);
    for (std::size_t index = 0; index < accessor.count; ++index) {
        const std::size_t offset = view.byteOffset + accessor.byteOffset + index * stride;
        values.push_back({
            readFloat(binary, offset),
            readFloat(binary, offset + sizeof(float)),
            readFloat(binary, offset + 2u * sizeof(float)),
        });
    }
    return values;
}

std::vector<std::uint32_t> readIndexAccessor(
    std::size_t accessorIndex,
    const std::vector<AccessorInfo>& accessors,
    const std::vector<BufferViewInfo>& views,
    std::span<const std::byte> binary) {
    if (accessorIndex >= accessors.size()) {
        throw std::runtime_error("GLB primitive references an invalid index accessor");
    }
    const auto& accessor = accessors[accessorIndex];
    if (accessor.type != "SCALAR") {
        throw std::runtime_error("GLB index accessor must be SCALAR");
    }

    std::size_t componentSize = 0u;
    switch (accessor.componentType) {
    case kUnsignedByteComponent: componentSize = 1u; break;
    case kUnsignedShortComponent: componentSize = 2u; break;
    case kUnsignedIntComponent: componentSize = 4u; break;
    default: throw std::runtime_error("GLB index accessor has unsupported component type");
    }

    const auto& view = views[accessor.bufferView];
    validateAccessorRange(accessor, view, componentSize);
    const std::size_t stride = view.byteStride == 0u ? componentSize : view.byteStride;
    std::vector<std::uint32_t> indices;
    indices.reserve(accessor.count);
    for (std::size_t index = 0; index < accessor.count; ++index) {
        const std::size_t offset = view.byteOffset + accessor.byteOffset + index * stride;
        std::uint32_t value = 0u;
        for (std::size_t byteIndex = 0; byteIndex < componentSize; ++byteIndex) {
            value |= std::to_integer<std::uint32_t>(binary[offset + byteIndex]) << (8u * byteIndex);
        }
        indices.push_back(value);
    }
    return indices;
}

std::vector<MeshMaterial> parseMaterials(const JsonValue& root) {
    const JsonValue* materialsValue = root.find("materials");
    if (materialsValue == nullptr) {
        return {};
    }

    std::vector<MeshMaterial> materials;
    materials.reserve(materialsValue->as_array().size());
    for (const auto& value : materialsValue->as_array()) {
        MeshMaterial material;
        if (const JsonValue* pbr = value.find("pbrMetallicRoughness")) {
            if (const JsonValue* color = pbr->find("baseColorFactor")) {
                const auto values = floatArray(*color, 4u, "material.baseColorFactor");
                material.baseColor = {values[0], values[1], values[2], values[3]};
            }
            if (const JsonValue* metallic = pbr->find("metallicFactor")) {
                material.metallic = asFloat(*metallic, "material.metallicFactor");
            }
            if (const JsonValue* roughness = pbr->find("roughnessFactor")) {
                material.roughness = asFloat(*roughness, "material.roughnessFactor");
            }
        }
        if (const JsonValue* alphaMode = value.find("alphaMode")) {
            material.translucent = alphaMode->as_string() == "BLEND";
        }
        material.translucent = material.translucent || material.baseColor.a < 0.999f;
        materials.push_back(material);
    }
    return materials;
}

void accumulateBounds(RuntimeMesh& mesh, Vec3 point, bool& hasBounds) {
    if (!hasBounds) {
        mesh.bounds = {point, point};
        hasBounds = true;
        return;
    }
    mesh.bounds.min.x = std::min(mesh.bounds.min.x, point.x);
    mesh.bounds.min.y = std::min(mesh.bounds.min.y, point.y);
    mesh.bounds.min.z = std::min(mesh.bounds.min.z, point.z);
    mesh.bounds.max.x = std::max(mesh.bounds.max.x, point.x);
    mesh.bounds.max.y = std::max(mesh.bounds.max.y, point.y);
    mesh.bounds.max.z = std::max(mesh.bounds.max.z, point.z);
}

void generateNormals(MeshPrimitive& primitive) {
    for (auto& vertex : primitive.vertices) {
        vertex.normal = {0.0f, 0.0f, 0.0f};
    }
    for (std::size_t index = 0; index < primitive.indices.size(); index += 3u) {
        const std::uint32_t i0 = primitive.indices[index];
        const std::uint32_t i1 = primitive.indices[index + 1u];
        const std::uint32_t i2 = primitive.indices[index + 2u];
        const Vec3 edge1 = primitive.vertices[i1].position - primitive.vertices[i0].position;
        const Vec3 edge2 = primitive.vertices[i2].position - primitive.vertices[i0].position;
        const Vec3 face = cross(edge1, edge2);
        primitive.vertices[i0].normal = primitive.vertices[i0].normal + face;
        primitive.vertices[i1].normal = primitive.vertices[i1].normal + face;
        primitive.vertices[i2].normal = primitive.vertices[i2].normal + face;
    }
    for (auto& vertex : primitive.vertices) {
        vertex.normal = normalize(vertex.normal);
    }
}

}  // namespace

RuntimeMesh loadGlbMesh(std::span<const std::byte> bytes) {
    if (bytes.size() < 20u) {
        throw std::runtime_error("GLB payload is too small");
    }
    if (readU32(bytes, 0u) != kGlbMagic) {
        throw std::runtime_error("invalid GLB magic");
    }
    if (readU32(bytes, 4u) != kGlbVersion) {
        throw std::runtime_error("unsupported GLB version");
    }
    const std::size_t declaredLength = readU32(bytes, 8u);
    if (declaredLength != bytes.size()) {
        throw std::runtime_error("GLB declared length does not match payload size");
    }

    std::string_view jsonText;
    std::span<const std::byte> binary;
    std::size_t offset = 12u;
    while (offset < declaredLength) {
        if (declaredLength - offset < 8u) {
            throw std::runtime_error("truncated GLB chunk header");
        }
        const std::size_t chunkLength = readU32(bytes, offset);
        const std::uint32_t chunkType = readU32(bytes, offset + 4u);
        offset += 8u;
        if (chunkLength > declaredLength - offset) {
            throw std::runtime_error("GLB chunk exceeds payload size");
        }
        if (chunkType == kJsonChunk) {
            if (!jsonText.empty()) {
                throw std::runtime_error("GLB contains multiple JSON chunks");
            }
            jsonText = std::string_view(
                reinterpret_cast<const char*>(bytes.data() + offset), chunkLength);
        } else if (chunkType == kBinChunk) {
            if (!binary.empty()) {
                throw std::runtime_error("GLB contains multiple BIN chunks");
            }
            binary = bytes.subspan(offset, chunkLength);
        }
        offset += chunkLength;
    }
    if (jsonText.empty() || binary.empty()) {
        throw std::runtime_error("GLB runtime requires JSON and embedded BIN chunks");
    }

    const JsonValue root = hh::assets::parse_json(jsonText);
    const auto& rootObject = root.as_object();
    static_cast<void>(rootObject);

    const auto& buffers = root.at("buffers").as_array();
    if (buffers.size() != 1u) {
        throw std::runtime_error("GLB runtime supports exactly one embedded buffer");
    }
    const std::size_t declaredBinaryLength = asIndex(buffers.front().at("byteLength"), "buffer.byteLength");
    if (declaredBinaryLength > binary.size()) {
        throw std::runtime_error("GLB buffer byteLength exceeds binary chunk");
    }

    const auto views = parseBufferViews(root, binary.size());
    const auto accessors = parseAccessors(root, views.size());
    RuntimeMesh result;
    result.materials = parseMaterials(root);
    bool hasBounds = false;
    std::size_t defaultMaterialIndex = std::numeric_limits<std::size_t>::max();

    const auto& meshes = root.at("meshes").as_array();
    const auto& nodes = root.at("nodes").as_array();
    std::vector<bool> active(nodes.size(), false);

    const auto ensureDefaultMaterial = [&result, &defaultMaterialIndex]() -> std::size_t {
        if (defaultMaterialIndex == std::numeric_limits<std::size_t>::max()) {
            defaultMaterialIndex = result.materials.size();
            result.materials.emplace_back();
        }
        return defaultMaterialIndex;
    };

    std::function<void(std::size_t, const Mat4&)> visitNode;
    visitNode = [&](std::size_t nodeIndex, const Mat4& parentTransform) {
        if (nodeIndex >= nodes.size()) {
            throw std::runtime_error("GLB scene references an invalid node");
        }
        if (active[nodeIndex]) {
            throw std::runtime_error("GLB node hierarchy contains a cycle");
        }
        active[nodeIndex] = true;
        const auto& node = nodes[nodeIndex];
        const Mat4 worldTransform = multiply(parentTransform, nodeLocalMatrix(node));

        if (const JsonValue* meshValue = node.find("mesh")) {
            const std::size_t meshIndex = asIndex(*meshValue, "node.mesh");
            if (meshIndex >= meshes.size()) {
                throw std::runtime_error("GLB node references an invalid mesh");
            }
            const auto& primitives = meshes[meshIndex].at("primitives").as_array();
            for (const auto& primitiveValue : primitives) {
                const std::uint32_t mode = primitiveValue.find("mode") == nullptr
                    ? kTrianglesMode
                    : asU32(*primitiveValue.find("mode"), "primitive.mode");
                if (mode != kTrianglesMode) {
                    throw std::runtime_error("GLB runtime supports triangle-list primitives only");
                }

                const auto& attributes = primitiveValue.at("attributes");
                const JsonValue* positionAccessor = attributes.find("POSITION");
                if (positionAccessor == nullptr) {
                    throw std::runtime_error("GLB primitive is missing POSITION");
                }
                auto positions = readVec3Accessor(
                    asIndex(*positionAccessor, "primitive.POSITION"), accessors, views, binary);
                if (positions.empty()) {
                    throw std::runtime_error("GLB primitive has no vertices");
                }

                std::vector<Vec3> normals;
                const JsonValue* normalAccessor = attributes.find("NORMAL");
                if (normalAccessor != nullptr) {
                    normals = readVec3Accessor(
                        asIndex(*normalAccessor, "primitive.NORMAL"), accessors, views, binary);
                    if (normals.size() != positions.size()) {
                        throw std::runtime_error("GLB NORMAL count does not match POSITION count");
                    }
                }

                MeshPrimitive primitive;
                primitive.vertices.resize(positions.size());
                for (std::size_t vertexIndex = 0; vertexIndex < positions.size(); ++vertexIndex) {
                    const Vec3 position = assetToRendererSpace(
                        transformPoint(worldTransform, positions[vertexIndex]));
                    primitive.vertices[vertexIndex].position = position;
                    if (!normals.empty()) {
                        const Vec3 normal = assetToRendererSpace(
                            transformNormal(worldTransform, normals[vertexIndex]));
                        primitive.vertices[vertexIndex].normal = normalize(normal);
                    } else {
                        primitive.vertices[vertexIndex].normal = {0.0f, 0.0f, 0.0f};
                    }
                    accumulateBounds(result, position, hasBounds);
                }

                if (const JsonValue* indicesValue = primitiveValue.find("indices")) {
                    primitive.indices = readIndexAccessor(
                        asIndex(*indicesValue, "primitive.indices"), accessors, views, binary);
                } else {
                    primitive.indices.reserve(positions.size());
                    for (std::size_t vertexIndex = 0; vertexIndex < positions.size(); ++vertexIndex) {
                        if (vertexIndex > std::numeric_limits<std::uint32_t>::max()) {
                            throw std::runtime_error("GLB primitive has too many vertices");
                        }
                        primitive.indices.push_back(static_cast<std::uint32_t>(vertexIndex));
                    }
                }
                if (primitive.indices.size() % 3u != 0u) {
                    throw std::runtime_error("GLB triangle primitive index count is not divisible by three");
                }
                for (const std::uint32_t index : primitive.indices) {
                    if (index >= primitive.vertices.size()) {
                        throw std::runtime_error("GLB primitive index is out of range");
                    }
                }
                for (std::size_t index = 0; index < primitive.indices.size(); index += 3u) {
                    std::swap(primitive.indices[index + 1u], primitive.indices[index + 2u]);
                }
                if (normals.empty()) {
                    generateNormals(primitive);
                }

                if (const JsonValue* materialValue = primitiveValue.find("material")) {
                    primitive.materialIndex = asIndex(*materialValue, "primitive.material");
                    if (primitive.materialIndex >= result.materials.size()) {
                        throw std::runtime_error("GLB primitive references an invalid material");
                    }
                } else {
                    primitive.materialIndex = ensureDefaultMaterial();
                }
                result.primitives.push_back(std::move(primitive));
            }
        }

        if (const JsonValue* children = node.find("children")) {
            for (const auto& child : children->as_array()) {
                visitNode(asIndex(child, "node.child"), worldTransform);
            }
        }
        active[nodeIndex] = false;
    };

    std::vector<std::size_t> roots;
    if (const JsonValue* scenesValue = root.find("scenes")) {
        const auto& scenes = scenesValue->as_array();
        if (scenes.empty()) {
            throw std::runtime_error("GLB scenes array is empty");
        }
        const std::size_t sceneIndex = root.find("scene") == nullptr
            ? 0u
            : asIndex(*root.find("scene"), "scene");
        if (sceneIndex >= scenes.size()) {
            throw std::runtime_error("GLB default scene index is out of range");
        }
        if (const JsonValue* sceneNodes = scenes[sceneIndex].find("nodes")) {
            for (const auto& node : sceneNodes->as_array()) {
                roots.push_back(asIndex(node, "scene.node"));
            }
        }
    } else {
        std::vector<bool> isChild(nodes.size(), false);
        for (const auto& node : nodes) {
            if (const JsonValue* children = node.find("children")) {
                for (const auto& child : children->as_array()) {
                    const std::size_t childIndex = asIndex(child, "node.child");
                    if (childIndex >= nodes.size()) {
                        throw std::runtime_error("GLB node references an invalid child");
                    }
                    isChild[childIndex] = true;
                }
            }
        }
        for (std::size_t nodeIndex = 0; nodeIndex < nodes.size(); ++nodeIndex) {
            if (!isChild[nodeIndex]) {
                roots.push_back(nodeIndex);
            }
        }
    }

    if (roots.empty()) {
        throw std::runtime_error("GLB has no scene root nodes");
    }
    const Mat4 identity = identityMatrix();
    for (const std::size_t rootIndex : roots) {
        visitNode(rootIndex, identity);
    }
    if (result.primitives.empty() || !hasBounds) {
        throw std::runtime_error("GLB scene contains no renderable mesh primitives");
    }
    return result;
}

}  // namespace hh::renderer
