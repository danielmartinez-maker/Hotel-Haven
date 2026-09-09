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

float viewDepth(
    const ComposedBox& box,
    const DirectX::XMMATRIX& viewMatrix) noexcept {
    const Vec3& center = box.item.center;
    const DirectX::XMVECTOR world =
        DirectX::XMVectorSet(center.x, center.y, center.z, 1.0f);
    const DirectX::XMVECTOR view = DirectX::XMVector4Transform(world, viewMatrix);
    return DirectX::XMVectorGetZ(view);
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

}  // namespace

ComposedScene prepareVisibleScene(
    const ComposedScene& scene,
    const OrthoCamera& camera) {
    ComposedScene visible;
    const DirectX::XMMATRIX viewProjection = camera.viewProjectionMatrix();

    appendVisible(scene.opaque, visible.opaque, viewProjection);
    appendVisible(scene.translucent, visible.translucent, viewProjection);
    appendVisible(scene.wireframe, visible.wireframe, viewProjection);

    const DirectX::XMMATRIX view = camera.viewMatrix();
    std::stable_sort(
        visible.translucent.begin(),
        visible.translucent.end(),
        [&view](const ComposedBox& lhs, const ComposedBox& rhs) {
            return viewDepth(lhs, view) > viewDepth(rhs, view);
        });

    return visible;
}

}  // namespace hh::renderer
