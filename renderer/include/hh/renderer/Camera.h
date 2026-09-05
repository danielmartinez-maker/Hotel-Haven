#pragma once

#include <DirectXMath.h>

#include "hh/renderer/MathTypes.h"

namespace hh::renderer {

class OrthoCamera {
public:
    OrthoCamera() noexcept;

    void setTarget(Vec3 target) noexcept;
    [[nodiscard]] Vec3 target() const noexcept;

    void setPitchDegrees(float value) noexcept;
    [[nodiscard]] float pitchDegrees() const noexcept;

    void setYawDegrees(float value) noexcept;
    [[nodiscard]] float yawDegrees() const noexcept;

    [[nodiscard]] static float snapYaw90(float value) noexcept;
    void rotateSnapped(int quarterTurns) noexcept;

    void setOrthoHeight(float value) noexcept;
    [[nodiscard]] float orthoHeight() const noexcept;

    void setAspectRatio(float value) noexcept;
    [[nodiscard]] float aspectRatio() const noexcept;

    [[nodiscard]] DirectX::XMMATRIX viewMatrix() const noexcept;
    [[nodiscard]] DirectX::XMMATRIX projectionMatrix() const noexcept;
    [[nodiscard]] DirectX::XMMATRIX viewProjectionMatrix() const noexcept;
    [[nodiscard]] Vec3 worldPosition() const noexcept;

private:
    Vec3 target_{};
    float pitchDegrees_{55.0f};
    float yawDegrees_{0.0f};
    float orthoHeight_{36.0f};
    float aspectRatio_{16.0f / 9.0f};
};

}  // namespace hh::renderer
