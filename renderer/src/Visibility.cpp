#include "hh/renderer/Visibility.h"

#include <DirectXMath.h>

#include <algorithm>
#include <array>
#include <vector>

namespace hh::renderer {
namespace {

bool aabbIntersectsClipVolume(
    const Aabb& bounds,
    const DirectX::XMMATRIX& viewProjection) noexcept {
    const std::array<Vec3, 8> corners{{
        {bounds.min.x, bounds.min.y, bounds.min.z},
        {bounds.min.x, bounds.min.y, bounds.max.z},
        {bounds.min.x, bounds.max.y, bounds.min.z},
        {bounds.min.x, bounds.max.y, bounds.max.z},
        {bounds.max.x, bounds.min.y, bounds.min.z},
        {bounds.max.x, bounds.min.y, bounds.max.z},
        {bounds.max.x, bounds.max.y, bounds.min.z},
        {bounds.max.x, bounds.max.y, bounds.max.z},
    }};

    std::array<DirectX::XMFLOAT4, 8> clipPoints{};
    for (std::size_t index = 0; index < corners.size(); ++index) {
        const Vec3& corner = corners[index];
        const DirectX::XMVECTOR world =
            DirectX::XMVectorSet(corner.x, corner.y, corner.z, 1.0f);
        const DirectX::XMVECTOR clip =
            DirectX::XMVector4Transform(world, viewProjection);
        DirectX::XMStoreFloat4(&clipPoints[index], clip);
    }

    const auto allOutside = [&clipPoints](const auto& predicate) {
        return std::all_of(clipPoints.begin(), clipPoints.end(), predicate);
    };

    if (allOutside([](const DirectX::XMFLOAT4& point) { return point.x < -point.w; }) ||
        allOutside([](const DirectX::XMFLOAT4& point) { return point.x > point.w; }) ||
        allOutside([](const DirectX::XMFLOAT4& point) { return point.y < -point.w; }) ||
        allOutside([](const DirectX::XMFLOAT4& point) { return point.y > point.w; }) ||
        allOutside([](const DirectX::XMFLOAT4& point) { return point.z < 0.0f; }) ||
        allOutside([](const DirectX::XMFLOAT4& point) { return point.z > point.w; })) {
        return false;
    }

    return true;
}

float pointViewDepth(Vec3 center, const DirectX::XMMATRIX& viewMatrix) noexcept {
    const DirectX::XMVECTOR world =
        DirectX::XMVectorSet(center.x, center.y, center.z, 1.0f);
    const DirectX::XMVECTOR view = DirectX::XMVector4Transform(world, viewMatrix);
    return DirectX::XMVectorGetZ(view);
}

float viewDepth(
    const ComposedBox& box,
    const DirectX::XMMATRIX& viewMatrix) noexcept {
    return pointViewDepth(box.item.center, viewMatrix);
}

float viewDepth(
    const ComposedMesh& mesh,
    const DirectX::XMMATRIX& viewMatrix) noexcept {
    const Aabb& bounds = mesh.item.bounds;
    return pointViewDepth({
        (bounds.min.x + bounds.max.x) * 0.5f,
        (bounds.min.y + bounds.max.y) * 0.5f,
        (bounds.min.z + bounds.max.z) * 0.5f,
    }, viewMatrix);
}

void appendVisible(
    const std::vector<ComposedBox>& source,
    std::vector<ComposedBox>& destination,
    const DirectX::XMMATRIX& viewProjection) {
    destination.reserve(source.size());
    for (const ComposedBox& box : source) {
        if (aabbIntersectsClipVolume(box.item.bounds, viewProjection)) {
            destination.push_back(box);
        }
    }
}

void appendVisible(
    const std::vector<ComposedMesh>& source,
    std::vector<ComposedMesh>& destination,
    const DirectX::XMMATRIX& viewProjection) {
    destination.reserve(source.size());
    for (const ComposedMesh& mesh : source) {
        if (aabbIntersectsClipVolume(mesh.item.bounds, viewProjection)) {
            destination.push_back(mesh);
        }
    }
}

}  // namespace

ComposedScene prepareVisibleScene(
    const ComposedScene& scene,
    const OrthoCamera& camera) {
    ComposedScene visible;
    const DirectX::XMMATRIX viewProjection = camera.viewProjectionMatrix();

    appendVisible(scene.opaque, visible.opaque, viewProjection);
    appendVisible(scene.translucent, visible.translucent, viewProjection);
    appendVisible(scene.wireframe, visible.wireframe, viewProjection);
    appendVisible(scene.opaqueMeshes, visible.opaqueMeshes, viewProjection);
    appendVisible(scene.translucentMeshes, visible.translucentMeshes, viewProjection);
    appendVisible(scene.wireframeMeshes, visible.wireframeMeshes, viewProjection);

    const DirectX::XMMATRIX view = camera.viewMatrix();
    std::stable_sort(
        visible.translucent.begin(),
        visible.translucent.end(),
        [&view](const ComposedBox& lhs, const ComposedBox& rhs) {
            return viewDepth(lhs, view) > viewDepth(rhs, view);
        });
    std::stable_sort(
        visible.translucentMeshes.begin(),
        visible.translucentMeshes.end(),
        [&view](const ComposedMesh& lhs, const ComposedMesh& rhs) {
            return viewDepth(lhs, view) > viewDepth(rhs, view);
        });

    return visible;
}

}  // namespace hh::renderer
