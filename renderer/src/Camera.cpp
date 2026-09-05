#include "hh/renderer/Camera.h"

#include <algorithm>
#include <cmath>

namespace hh::renderer {
namespace {

constexpr float kMinPitch = 45.0f;
constexpr float kMaxPitch = 65.0f;
constexpr float kViewDistance = 100.0f;
constexpr float kMinimumPositive = 0.001f;

float normalizeDegrees(float value) noexcept {
    float normalized = std::fmod(value, 360.0f);
    if (normalized < 0.0f) {
        normalized += 360.0f;
    }
    return normalized;
}

}  // namespace

OrthoCamera::OrthoCamera() noexcept = default;

void OrthoCamera::setTarget(Vec3 target) noexcept { target_ = target; }
Vec3 OrthoCamera::target() const noexcept { return target_; }

void OrthoCamera::setPitchDegrees(float value) noexcept {
    pitchDegrees_ = std::clamp(value, kMinPitch, kMaxPitch);
}

float OrthoCamera::pitchDegrees() const noexcept { return pitchDegrees_; }

void OrthoCamera::setYawDegrees(float value) noexcept { yawDegrees_ = normalizeDegrees(value); }
float OrthoCamera::yawDegrees() const noexcept { return yawDegrees_; }

float OrthoCamera::snapYaw90(float value) noexcept {
    const float normalized = normalizeDegrees(value);
    const float snapped = std::round(normalized / 90.0f) * 90.0f;
    return normalizeDegrees(snapped);
}

void OrthoCamera::rotateSnapped(int quarterTurns) noexcept {
    setYawDegrees(snapYaw90(yawDegrees_) + static_cast<float>(quarterTurns) * 90.0f);
}

void OrthoCamera::setOrthoHeight(float value) noexcept {
    orthoHeight_ = std::max(value, kMinimumPositive);
}

float OrthoCamera::orthoHeight() const noexcept { return orthoHeight_; }

void OrthoCamera::setAspectRatio(float value) noexcept {
    aspectRatio_ = std::max(value, kMinimumPositive);
}

float OrthoCamera::aspectRatio() const noexcept { return aspectRatio_; }

Vec3 OrthoCamera::worldPosition() const noexcept {
    const float pitch = DirectX::XMConvertToRadians(pitchDegrees_);
    const float yaw = DirectX::XMConvertToRadians(yawDegrees_);
    const float horizontal = std::cos(pitch) * kViewDistance;
    return {
        target_.x + std::sin(yaw) * horizontal,
        target_.y + std::sin(pitch) * kViewDistance,
        target_.z - std::cos(yaw) * horizontal,
    };
}

DirectX::XMMATRIX OrthoCamera::viewMatrix() const noexcept {
    const Vec3 eye = worldPosition();
    const auto eyeVector = DirectX::XMVectorSet(eye.x, eye.y, eye.z, 1.0f);
    const auto targetVector = DirectX::XMVectorSet(target_.x, target_.y, target_.z, 1.0f);
    const auto upVector = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    return DirectX::XMMatrixLookAtLH(eyeVector, targetVector, upVector);
}

DirectX::XMMATRIX OrthoCamera::projectionMatrix() const noexcept {
    return DirectX::XMMatrixOrthographicLH(
        orthoHeight_ * aspectRatio_, orthoHeight_, 0.1f, 2000.0f);
}

DirectX::XMMATRIX OrthoCamera::viewProjectionMatrix() const noexcept {
    return DirectX::XMMatrixMultiply(viewMatrix(), projectionMatrix());
}

}  // namespace hh::renderer
