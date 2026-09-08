#include "TestFramework.h"

#include "d3d11/D3D11Renderer.h"
#include "win32/Win32Window.h"

#include <filesystem>
#include <string>

namespace {

hh::renderer::ComposedScene makeScene() {
    using namespace hh::renderer;
    ComposedScene scene;

    ComposedBox opaque;
    opaque.item.center = {0.0f, 0.5f, 0.0f};
    opaque.item.size = {2.0f, 1.0f, 2.0f};
    opaque.item.color = {0.7f, 0.6f, 0.5f, 1.0f};
    scene.opaque.push_back(opaque);

    ComposedBox translucent;
    translucent.item.center = {1.5f, 1.0f, 0.0f};
    translucent.item.size = {0.5f, 2.0f, 2.0f};
    translucent.item.color = {0.3f, 0.6f, 0.8f, 0.35f};
    scene.translucent.push_back(translucent);

    ComposedBox wireframe;
    wireframe.item.center = {-1.5f, 0.5f, 0.0f};
    wireframe.item.size = {0.75f, 1.0f, 0.75f};
    wireframe.item.color = {0.9f, 0.9f, 0.2f, 0.8f};
    scene.wireframe.push_back(wireframe);

    return scene;
}

std::filesystem::path shaderPath() {
    return std::filesystem::path(__FILE__).parent_path().parent_path() /
           "shaders" / "InstancedBox.hlsl";
}

}  // namespace

TEST_CASE("D3D11 renderer survives hidden-window initialize render resize shutdown cycles") {
    using namespace hh::renderer;

    HINSTANCE instance = GetModuleHandleW(nullptr);
    EXPECT_TRUE(instance != nullptr);

    std::string windowError;
    Win32Window window;
    EXPECT_TRUE(window.create(instance, SW_HIDE, 320, 240, windowError));
    EXPECT_TRUE(window.handle() != nullptr);

    const ComposedScene scene = makeScene();
    OrthoCamera camera;
    camera.setTarget({0.0f, 0.0f, 0.0f});
    camera.setPitchDegrees(45.0f);
    camera.setYawDegrees(45.0f);
    camera.setOrthoHeight(12.0f);
    camera.setAspectRatio(320.0f / 240.0f);

    D3D11Renderer renderer;
    for (int cycle = 0; cycle < 3; ++cycle) {
        const RendererResult initialized = renderer.initialize(window.handle(), 320, 240, shaderPath());
        EXPECT_TRUE(initialized.succeeded);

        EXPECT_TRUE(renderer.render(scene, camera).succeeded);
        EXPECT_TRUE(renderer.resize(0, 0).succeeded);
        EXPECT_TRUE(renderer.render(scene, camera).succeeded);

        const std::uint32_t width = static_cast<std::uint32_t>(256 + cycle * 64);
        const std::uint32_t height = static_cast<std::uint32_t>(192 + cycle * 48);
        EXPECT_TRUE(renderer.resize(width, height).succeeded);
        camera.setAspectRatio(static_cast<float>(width) / static_cast<float>(height));
        EXPECT_TRUE(renderer.render(scene, camera).succeeded);

        renderer.shutdown();
        EXPECT_FALSE(renderer.render(scene, camera).succeeded);
        EXPECT_FALSE(renderer.resize(width, height).succeeded);
    }
}
