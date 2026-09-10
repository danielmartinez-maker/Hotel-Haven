#include "TestFramework.h"
#include "d3d11/D3D11Renderer.h"
#include "hh/renderer/Camera.h"
#include "hh/renderer/RenderScene.h"

#include <windows.h>

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

std::size_t lifecycleBudget() {
    const char* scale = std::getenv("HH_STRESS_SCALE");
    const std::string_view value = scale ? std::string_view{scale} : std::string_view{"pr"};
    if (value == "pr") return 100;
    if (value == "extended") return 500;
    if (value == "exhaustive") return 1'000;
    throw std::runtime_error("HH_STRESS_SCALE must be pr, extended, or exhaustive");
}

LRESULT CALLBACK stressWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    return DefWindowProcW(window, message, wParam, lParam);
}

class StressWindow {
public:
    explicit StressWindow(std::uint64_t serial) {
        instance_ = GetModuleHandleW(nullptr);
        className_ = L"HotelHavenRendererStressWindow_" + std::to_wstring(serial);
        WNDCLASSW windowClass{};
        windowClass.lpfnWndProc = stressWindowProc;
        windowClass.hInstance = instance_;
        windowClass.lpszClassName = className_.c_str();
        atom_ = RegisterClassW(&windowClass);
        if (atom_ == 0)
            return;
        window_ = CreateWindowExW(0, className_.c_str(), L"", WS_OVERLAPPEDWINDOW,
                                  0, 0, 96, 96, nullptr, nullptr, instance_, nullptr);
    }

    ~StressWindow() {
        if (window_ != nullptr)
            DestroyWindow(window_);
        if (atom_ != 0)
            UnregisterClassW(className_.c_str(), instance_);
    }

    [[nodiscard]] HWND get() const noexcept { return window_; }

private:
    HINSTANCE instance_{};
    std::wstring className_;
    ATOM atom_{};
    HWND window_{};
};

std::filesystem::path shaderPath(std::string_view fileName) {
    return std::filesystem::path(__FILE__).parent_path().parent_path() /
           "shaders" / fileName;
}

hh::renderer::ComposedScene oneBoxScene(std::size_t cycle) {
    using namespace hh::renderer;
    const float x = static_cast<float>(cycle % 8);
    const Vec3 center{x, 1.0f, 0.0f};
    BoxRenderItem item{
        center,
        {1.0f, 2.0f, 1.0f},
        {0.7f, 0.7f, 0.7f, 1.0f},
        0,
        RenderCategory::Object,
        {{x - 0.5f, 0.0f, -0.5f}, {x + 0.5f, 2.0f, 0.5f}}};
    ComposedScene scene;
    scene.opaque.push_back({item, false});
    return scene;
}
}  // namespace

TEST_CASE("D3D11 WARP renderer survives repeated initialize render present shutdown lifecycle") {
    using namespace hh::renderer;
    const std::size_t cycles = lifecycleBudget();
    for (std::size_t cycle = 0; cycle < cycles; ++cycle) {
        StressWindow window(cycle + 1);
        EXPECT_TRUE(window.get() != nullptr);

        D3D11Renderer renderer;
        const auto initialized = renderer.initialize(
            window.get(), 96u, 96u, shaderPath("InstancedBox.hlsl"), true);
        EXPECT_TRUE(initialized);
        EXPECT_TRUE(renderer.device() != nullptr);
        EXPECT_TRUE(renderer.swapChain() != nullptr);

        OrthoCamera camera;
        camera.setAspectRatio(1.0f);
        camera.setOrthoHeight(12.0f);
        camera.setTarget({4.0f, 1.0f, 0.0f});
        const auto scene = oneBoxScene(cycle);
        EXPECT_TRUE(renderer.renderWorld(scene, camera));
        EXPECT_TRUE(renderer.present());
        EXPECT_EQ(renderer.stats().boxDrawCalls, 1u);

        renderer.shutdown();
        EXPECT_TRUE(renderer.device() == nullptr);
        EXPECT_TRUE(renderer.swapChain() == nullptr);
        EXPECT_FALSE(renderer.renderWorld(scene, camera));
        EXPECT_FALSE(renderer.present());
    }
}
