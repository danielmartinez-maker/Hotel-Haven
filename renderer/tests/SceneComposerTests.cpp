#include "TestFramework.h"
#include "hh/renderer/SceneComposer.h"

namespace {

hh::renderer::BoxRenderItem makeBox(int floorId,
                                    hh::renderer::RenderCategory category,
                                    hh::renderer::Vec3 center,
                                    hh::renderer::Vec3 size) {
    using namespace hh::renderer;
    const Vec3 half{size.x * 0.5f, size.y * 0.5f, size.z * 0.5f};
    return BoxRenderItem{
        center,
        size,
        Color{0.8f, 0.8f, 0.8f, 1.0f},
        floorId,
        category,
        Aabb{{center.x - half.x, center.y - half.y, center.z - half.z},
             {center.x + half.x, center.y + half.y, center.z + half.z}}
    };
}

hh::renderer::MeshRenderItem makeMesh(int floorId,
                                      hh::renderer::RenderCategory category,
                                      hh::renderer::Vec3 center,
                                      bool translucent = false) {
    using namespace hh::renderer;
    return MeshRenderItem{
        AssetHandle{7u},
        MeshTransform{center, Vec3{1.0f, 1.0f, 1.0f}, 0.0f},
        Color{1.0f, 1.0f, 1.0f, 1.0f},
        floorId,
        category,
        Aabb{{center.x - 0.5f, center.y - 0.5f, center.z - 0.5f},
             {center.x + 0.5f, center.y + 0.5f, center.z + 0.5f}},
        translucent
    };
}

}  // namespace

TEST_CASE("scene composer excludes hidden floors") {
    using namespace hh::renderer;
    RenderScene scene;
    scene.activeFloor = 1;
    scene.items.push_back(makeBox(1, RenderCategory::Object, {0, 1, 0}, {1, 1, 1}));
    scene.items.push_back(makeBox(0, RenderCategory::Object, {0, 0, 0}, {1, 1, 1}));

    const auto result = SceneComposer{}.compose(scene, FloorContextMode::Normal, WallRenderMode::FullHeight, {0, 5, -10});
    EXPECT_EQ(result.opaque.size(), static_cast<std::size_t>(1));
    EXPECT_TRUE(result.translucent.empty());
}

TEST_CASE("scene composer makes adjacent context floor translucent") {
    using namespace hh::renderer;
    RenderScene scene;
    scene.activeFloor = 1;
    scene.items.push_back(makeBox(0, RenderCategory::Object, {0, 0, 0}, {1, 1, 1}));

    const auto result = SceneComposer{}.compose(scene, FloorContextMode::AdjacentContext, WallRenderMode::FullHeight, {0, 5, -10});
    EXPECT_EQ(result.translucent.size(), static_cast<std::size_t>(1));
    EXPECT_NEAR(result.translucent.front().item.color.a, 0.25f, 0.0001f);
}

TEST_CASE("scene composer routes blueprint walls to wireframe") {
    using namespace hh::renderer;
    RenderScene scene;
    scene.activeFloor = 0;
    scene.items.push_back(makeBox(0, RenderCategory::Wall, {0, 1, 0}, {4, 2, 0.2f}));

    const auto result = SceneComposer{}.compose(scene, FloorContextMode::Normal, WallRenderMode::Blueprint, {0, 5, -10});
    EXPECT_EQ(result.wireframe.size(), static_cast<std::size_t>(1));
    EXPECT_TRUE(result.opaque.empty());
}

TEST_CASE("focus protection cuts only an occluding full-height wall") {
    using namespace hh::renderer;
    RenderScene scene;
    scene.activeFloor = 0;
    scene.focusTarget = Vec3{0, 1, 0};
    scene.items.push_back(makeBox(0, RenderCategory::Wall, {0, 1.5f, -5}, {2, 3, 0.4f}));
    scene.items.push_back(makeBox(0, RenderCategory::Wall, {10, 1.5f, -5}, {2, 3, 0.4f}));

    const auto result = SceneComposer{}.compose(scene, FloorContextMode::Normal, WallRenderMode::FullHeight, {0, 5, -10});
    EXPECT_EQ(result.opaque.size(), static_cast<std::size_t>(2));
    EXPECT_TRUE(result.opaque[0].cutaway);
    EXPECT_FALSE(result.opaque[1].cutaway);
}

TEST_CASE("scene composer filters and classifies mesh instances without registry lookup") {
    using namespace hh::renderer;
    RenderScene scene;
    scene.activeFloor = 1;
    scene.meshes.push_back(makeMesh(1, RenderCategory::Object, {0.0f, 1.0f, 0.0f}));
    scene.meshes.push_back(makeMesh(1, RenderCategory::Object, {2.0f, 1.0f, 0.0f}, true));
    scene.meshes.push_back(makeMesh(0, RenderCategory::Object, {4.0f, 0.0f, 0.0f}));

    const auto result = SceneComposer{}.compose(
        scene, FloorContextMode::Normal, WallRenderMode::FullHeight, {0, 5, -10});

    EXPECT_EQ(result.opaqueMeshes.size(), 1u);
    EXPECT_EQ(result.translucentMeshes.size(), 1u);
    EXPECT_EQ(result.opaqueMeshes.front().item.asset.value, 7u);
}

TEST_CASE("scene composer fades mesh instances on adjacent context floors") {
    using namespace hh::renderer;
    RenderScene scene;
    scene.activeFloor = 1;
    scene.meshes.push_back(makeMesh(0, RenderCategory::Object, {0.0f, 0.0f, 0.0f}));

    const auto result = SceneComposer{}.compose(
        scene, FloorContextMode::AdjacentContext, WallRenderMode::FullHeight, {0, 5, -10});

    EXPECT_EQ(result.translucentMeshes.size(), 1u);
    EXPECT_NEAR(result.translucentMeshes.front().item.tint.a, 0.25f, 0.0001f);
}
