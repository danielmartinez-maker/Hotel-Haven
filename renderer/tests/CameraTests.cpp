#include "TestFramework.h"
#include "hh/renderer/Camera.h"

TEST_CASE("camera defaults to HMG-000 pitch") {
    hh::renderer::OrthoCamera camera;
    EXPECT_NEAR(camera.pitchDegrees(), 55.0f, 0.0001f);
}

TEST_CASE("camera clamps pitch to HMG-000 range") {
    hh::renderer::OrthoCamera camera;
    camera.setPitchDegrees(10.0f);
    EXPECT_NEAR(camera.pitchDegrees(), 45.0f, 0.0001f);
    camera.setPitchDegrees(90.0f);
    EXPECT_NEAR(camera.pitchDegrees(), 65.0f, 0.0001f);
}

TEST_CASE("camera normalizes yaw and snaps to quarter turns") {
    hh::renderer::OrthoCamera camera;
    camera.setYawDegrees(361.0f);
    EXPECT_NEAR(camera.yawDegrees(), 1.0f, 0.0001f);
    EXPECT_NEAR(hh::renderer::OrthoCamera::snapYaw90(136.0f), 180.0f, 0.0001f);
    camera.setYawDegrees(0.0f);
    camera.rotateSnapped(-1);
    EXPECT_NEAR(camera.yawDegrees(), 270.0f, 0.0001f);
}
