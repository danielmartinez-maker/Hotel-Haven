#include "TestFramework.h"
#include "hh/renderer/Visibility.h"

#include <cmath>

namespace {

hh::renderer::ComposedBox makeBox(hh::renderer::Vec3 center,
                                  hh::renderer::Vec3 size = {1.0f, 1.0f, 1.0f}) {
    using namespace hh::renderer;
    const Vec3 half{size.x * 0.5f, size.y * 0.5f, size.z * 0.5f};
    BoxRenderItem item{
        center,
        size,
        Color{0.8f, 0.8f, 0.8f, 0.5f},
        0,
        RenderCategory::Object,
        Aabb{{center.x - half.x, center.y - half.y, center.z - half.z},
             {center.x + half.x, center.y + half.y, center.z + half.z}}
    };
    return ComposedBox{item, false};
}

hh::renderer::Vec3 pointAlongView(const hh::renderer::OrthoCamera& camera, float distance) {
    using namespace hh::renderer;
    const Vec3 eye = camera.worldPosition();
    const Vec3 target = camera.target();
    const float dx = target.x - eye.x;
    const float dy = target.y - eye.y;
    const float dz = target.z - eye.z;
    const float length = std::sqrt(dx * dx + dy * dy + dz * dz);
    return {
        eye.x + dx / length * distance,
        eye.y + dy / length * distance,
        eye.z + dz / length * distance,
    };
}

}  // namespace

TEST_CASE("visibility stage culls boxes outside the orthographic camera frustum") {
    using namespace hh::renderer;
    OrthoCamera camera;
    camera.setAspectRatio(1.0f);
    camera.setOrthoHeight(20.0f);

    ComposedScene scene;
    scene.opaque.push_back(makeBox({0.0f, 0.0f, 0.0f}));
    scene.opaque.push_back(makeBox({1000.0f, 0.0f, 0.0f}));

    const ComposedScene visible = prepareVisibleScene(scene, camera);

    EXPECT_EQ(visible.opaque.size(), static_cast<std::size_t>(1));
    EXPECT_NEAR(visible.opaque.front().item.center.x, 0.0f, 0.0001f);
}

TEST_CASE("visibility stage sorts translucent boxes back to front") {
    using namespace hh::renderer;
    OrthoCamera camera;
    camera.setAspectRatio(1.0f);
    camera.setOrthoHeight(20.0f);

    const Vec3 nearCenter = pointAlongView(camera, 20.0f);
    const Vec3 farCenter = pointAlongView(camera, 80.0f);

    ComposedScene scene;
    scene.translucent.push_back(makeBox(nearCenter));
    scene.translucent.push_back(makeBox(farCenter));

    const ComposedScene visible = prepareVisibleScene(scene, camera);

    EXPECT_EQ(visible.translucent.size(), static_cast<std::size_t>(2));
    EXPECT_NEAR(visible.translucent[0].item.center.y, farCenter.y, 0.0001f);
    EXPECT_NEAR(visible.translucent[0].item.center.z, farCenter.z, 0.0001f);
    EXPECT_NEAR(visible.translucent[1].item.center.y, nearCenter.y, 0.0001f);
    EXPECT_NEAR(visible.translucent[1].item.center.z, nearCenter.z, 0.0001f);
}

TEST_CASE("visibility stage preserves translucent order at equal view depth") {
    using namespace hh::renderer;
    OrthoCamera camera;
    camera.setAspectRatio(1.0f);
    camera.setOrthoHeight(20.0f);

    const Vec3 center = pointAlongView(camera, 60.0f);
    ComposedScene scene;
    scene.translucent.push_back(makeBox({center.x - 1.0f, center.y, center.z}));
    scene.translucent.push_back(makeBox({center.x + 1.0f, center.y, center.z}));

    const ComposedScene visible = prepareVisibleScene(scene, camera);

    EXPECT_EQ(visible.translucent.size(), static_cast<std::size_t>(2));
    EXPECT_NEAR(visible.translucent[0].item.center.x, center.x - 1.0f, 0.0001f);
    EXPECT_NEAR(visible.translucent[1].item.center.x, center.x + 1.0f, 0.0001f);
}
