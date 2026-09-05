#include <windows.h>

#include <DirectXMath.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <optional>
#include <sstream>
#include <string>

#include "d3d11/D3D11Renderer.h"
#include "hh/renderer/SceneComposer.h"
#include "win32/Win32Window.h"

namespace {

using hh::renderer::Aabb;
using hh::renderer::BoxRenderItem;
using hh::renderer::Color;
using hh::renderer::FloorContextMode;
using hh::renderer::OrthoCamera;
using hh::renderer::RenderCategory;
using hh::renderer::RenderScene;
using hh::renderer::Vec3;
using hh::renderer::WallRenderMode;

constexpr float kDemoFloorHeight = 3.2f;
constexpr float kDemoWallHeight = 2.8f;
constexpr float kDemoWallThickness = 0.15f;
constexpr int kDemoMinFloor = 0;
constexpr int kDemoMaxFloor = 2;
constexpr float kDemoMinOrthoHeight = 8.0f;
constexpr float kDemoMaxOrthoHeight = 120.0f;

Aabb boundsFor(Vec3 center, Vec3 size) {
    const Vec3 half{size.x * 0.5f, size.y * 0.5f, size.z * 0.5f};
    return {
        {center.x - half.x, center.y - half.y, center.z - half.z},
        {center.x + half.x, center.y + half.y, center.z + half.z},
    };
}

BoxRenderItem makeBox(
    int floor,
    RenderCategory category,
    Vec3 center,
    Vec3 size,
    Color color) {
    return BoxRenderItem{center, size, color, floor, category, boundsFor(center, size)};
}

RenderScene buildDemoScene() {
    RenderScene scene;
    scene.activeFloor = 0;
    scene.items.reserve(64);

    const Color floorColor{0.45f, 0.48f, 0.52f, 1.0f};
    const Color wallColor{0.82f, 0.80f, 0.74f, 1.0f};
    const Color objectColor{0.28f, 0.48f, 0.62f, 1.0f};

    for (int floor = kDemoMinFloor; floor <= kDemoMaxFloor; ++floor) {
        const float baseY = static_cast<float>(floor) * kDemoFloorHeight;
        const float wallCenterY = baseY + kDemoWallHeight * 0.5f;

        scene.items.push_back(makeBox(
            floor,
            RenderCategory::Floor,
            {0.0f, baseY - 0.10f, 0.0f},
            {12.0f, 0.20f, 10.0f},
            floorColor));

        scene.items.push_back(makeBox(floor, RenderCategory::Wall,
            {0.0f, wallCenterY, -5.0f}, {12.0f, kDemoWallHeight, kDemoWallThickness}, wallColor));
        scene.items.push_back(makeBox(floor, RenderCategory::Wall,
            {0.0f, wallCenterY, 5.0f}, {12.0f, kDemoWallHeight, kDemoWallThickness}, wallColor));
        scene.items.push_back(makeBox(floor, RenderCategory::Wall,
            {-6.0f, wallCenterY, 0.0f}, {kDemoWallThickness, kDemoWallHeight, 10.0f}, wallColor));
        scene.items.push_back(makeBox(floor, RenderCategory::Wall,
            {6.0f, wallCenterY, 0.0f}, {kDemoWallThickness, kDemoWallHeight, 10.0f}, wallColor));

        scene.items.push_back(makeBox(floor, RenderCategory::Wall,
            {-3.25f, wallCenterY, 0.0f}, {5.5f, kDemoWallHeight, kDemoWallThickness}, wallColor));
        scene.items.push_back(makeBox(floor, RenderCategory::Wall,
            {3.25f, wallCenterY, 0.0f}, {5.5f, kDemoWallHeight, kDemoWallThickness}, wallColor));

        scene.items.push_back(makeBox(floor, RenderCategory::Object,
            {-3.5f, baseY + 0.35f, 2.5f}, {2.2f, 0.7f, 1.2f}, objectColor));
        scene.items.push_back(makeBox(floor, RenderCategory::Object,
            {3.5f, baseY + 0.35f, 2.5f}, {2.2f, 0.7f, 1.2f}, objectColor));
        scene.items.push_back(makeBox(floor, RenderCategory::Object,
            {0.0f, baseY + 0.55f, -2.5f}, {3.0f, 1.1f, 0.8f}, objectColor));
    }

    return scene;
}

std::filesystem::path shaderPathFromExecutable() {
    std::array<wchar_t, 32768> buffer{};
    const DWORD length = GetModuleFileNameW(
        nullptr,
        buffer.data(),
        static_cast<DWORD>(buffer.size()));
    if (length == 0 || length >= buffer.size()) {
        return {};
    }
    return std::filesystem::path(buffer.data(), buffer.data() + length)
               .parent_path() /
           L"shaders" /
           L"InstancedBox.hlsl";
}

void showFatalError(const std::string& message) {
    OutputDebugStringA(message.c_str());
    OutputDebugStringA("\n");
    const std::wstring wide(message.begin(), message.end());
    MessageBoxW(nullptr, wide.c_str(), L"Hotel Haven Renderer Error", MB_OK | MB_ICONERROR);
}

std::optional<Vec3> pickActiveFloor(
    POINT cursor,
    std::uint32_t width,
    std::uint32_t height,
    float planeY,
    const OrthoCamera& camera) {
    if (width == 0 || height == 0) {
        return std::nullopt;
    }

    const DirectX::XMMATRIX identity = DirectX::XMMatrixIdentity();
    const DirectX::XMVECTOR nearPoint = DirectX::XMVector3Unproject(
        DirectX::XMVectorSet(
            static_cast<float>(cursor.x),
            static_cast<float>(cursor.y),
            0.0f,
            1.0f),
        0.0f,
        0.0f,
        static_cast<float>(width),
        static_cast<float>(height),
        0.0f,
        1.0f,
        camera.projectionMatrix(),
        camera.viewMatrix(),
        identity);
    const DirectX::XMVECTOR farPoint = DirectX::XMVector3Unproject(
        DirectX::XMVectorSet(
            static_cast<float>(cursor.x),
            static_cast<float>(cursor.y),
            1.0f,
            1.0f),
        0.0f,
        0.0f,
        static_cast<float>(width),
        static_cast<float>(height),
        0.0f,
        1.0f,
        camera.projectionMatrix(),
        camera.viewMatrix(),
        identity);

    DirectX::XMFLOAT3 nearWorld{};
    DirectX::XMFLOAT3 farWorld{};
    DirectX::XMStoreFloat3(&nearWorld, nearPoint);
    DirectX::XMStoreFloat3(&farWorld, farPoint);

    const float deltaY = farWorld.y - nearWorld.y;
    if (std::fabs(deltaY) < 1.0e-6f) {
        return std::nullopt;
    }

    const float t = (planeY - nearWorld.y) / deltaY;
    if (t < 0.0f || t > 1.0f) {
        return std::nullopt;
    }

    return Vec3{
        nearWorld.x + (farWorld.x - nearWorld.x) * t,
        planeY,
        nearWorld.z + (farWorld.z - nearWorld.z) * t,
    };
}

const wchar_t* wallModeName(WallRenderMode mode) {
    switch (mode) {
    case WallRenderMode::FullHeight:
        return L"Full";
    case WallRenderMode::Cutaway:
        return L"Cutaway";
    case WallRenderMode::Blueprint:
        return L"Blueprint";
    }
    return L"Unknown";
}

void updateWindowTitle(
    hh::renderer::Win32Window& window,
    int activeFloor,
    FloorContextMode contextMode,
    WallRenderMode wallMode) {
    std::wostringstream stream;
    stream << L"Hotel Haven — Renderer Foundation C | Floor " << activeFloor
           << L" | Context "
           << (contextMode == FloorContextMode::AdjacentContext ? L"On" : L"Off")
           << L" | Walls " << wallModeName(wallMode);
    window.setTitle(stream.str());
}

}  // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    hh::renderer::Win32Window window;
    std::string windowError;
    if (!window.create(instance, showCommand, 1600, 900, windowError)) {
        showFatalError(windowError);
        return 1;
    }

    OrthoCamera camera;
    camera.setTarget({0.0f, 0.0f, 0.0f});
    camera.setOrthoHeight(28.0f);
    if (window.clientHeight() != 0) {
        camera.setAspectRatio(
            static_cast<float>(window.clientWidth()) /
            static_cast<float>(window.clientHeight()));
    }

    hh::renderer::D3D11Renderer renderer;
    const std::filesystem::path shaderPath = shaderPathFromExecutable();
    hh::renderer::RendererResult rendererResult = renderer.initialize(
        window.handle(),
        window.clientWidth(),
        window.clientHeight(),
        shaderPath);
    if (!rendererResult) {
        showFatalError(rendererResult.error);
        return 2;
    }

    const RenderScene baseScene = buildDemoScene();
    hh::renderer::SceneComposer composer;
    int activeFloor = 0;
    FloorContextMode contextMode = FloorContextMode::Normal;
    WallRenderMode wallMode = WallRenderMode::FullHeight;
    std::optional<Vec3> focusTarget;
    updateWindowTitle(window, activeFloor, contextMode, wallMode);

    auto previousTime = std::chrono::steady_clock::now();
    while (window.pumpMessages()) {
        const auto currentTime = std::chrono::steady_clock::now();
        const float deltaSeconds = std::clamp(
            std::chrono::duration<float>(currentTime - previousTime).count(),
            0.0f,
            0.1f);
        previousTime = currentTime;

        if (window.consumeKeyPressed(VK_ESCAPE)) {
            window.requestClose();
        }
        if (window.consumeKeyPressed('Q')) {
            camera.rotateSnapped(-1);
        }
        if (window.consumeKeyPressed('E')) {
            camera.rotateSnapped(1);
        }
        if (window.consumeKeyPressed('C')) {
            contextMode = contextMode == FloorContextMode::Normal
                              ? FloorContextMode::AdjacentContext
                              : FloorContextMode::Normal;
        }
        if (window.consumeKeyPressed('1')) {
            wallMode = WallRenderMode::FullHeight;
        }
        if (window.consumeKeyPressed('2')) {
            wallMode = WallRenderMode::Cutaway;
        }
        if (window.consumeKeyPressed('3')) {
            wallMode = WallRenderMode::Blueprint;
        }

        int requestedFloor = activeFloor;
        if (window.consumeKeyPressed(VK_PRIOR)) {
            ++requestedFloor;
        }
        if (window.consumeKeyPressed(VK_NEXT)) {
            --requestedFloor;
        }
        requestedFloor = std::clamp(requestedFloor, kDemoMinFloor, kDemoMaxFloor);
        if (requestedFloor != activeFloor) {
            activeFloor = requestedFloor;
            Vec3 target = camera.target();
            target.y = static_cast<float>(activeFloor) * kDemoFloorHeight;
            camera.setTarget(target);
            focusTarget.reset();
        }

        const float yaw = DirectX::XMConvertToRadians(camera.yawDegrees());
        const Vec3 forward{std::sin(yaw), 0.0f, std::cos(yaw)};
        const Vec3 right{std::cos(yaw), 0.0f, -std::sin(yaw)};
        Vec3 movement{};
        if (window.isKeyDown('W')) {
            movement = movement + forward;
        }
        if (window.isKeyDown('S')) {
            movement = movement - forward;
        }
        if (window.isKeyDown('D')) {
            movement = movement + right;
        }
        if (window.isKeyDown('A')) {
            movement = movement - right;
        }
        const float movementLength = std::sqrt(
            movement.x * movement.x + movement.z * movement.z);
        if (movementLength > 1.0e-6f) {
            movement = movement * (1.0f / movementLength);
            Vec3 target = camera.target();
            const float panSpeed = camera.orthoHeight() * 0.70f;
            target = target + movement * (panSpeed * deltaSeconds);
            target.y = static_cast<float>(activeFloor) * kDemoFloorHeight;
            camera.setTarget(target);
        }

        const int wheelDelta = window.consumeMouseWheelDelta();
        if (wheelDelta != 0) {
            const float wheelSteps = static_cast<float>(wheelDelta) /
                                     static_cast<float>(WHEEL_DELTA);
            const float zoomFactor = std::pow(0.85f, wheelSteps);
            camera.setOrthoHeight(std::clamp(
                camera.orthoHeight() * zoomFactor,
                kDemoMinOrthoHeight,
                kDemoMaxOrthoHeight));
        }

        std::uint32_t resizedWidth{};
        std::uint32_t resizedHeight{};
        if (window.consumeResize(resizedWidth, resizedHeight)) {
            if (resizedHeight != 0) {
                camera.setAspectRatio(
                    static_cast<float>(resizedWidth) /
                    static_cast<float>(resizedHeight));
            }
            rendererResult = renderer.resize(resizedWidth, resizedHeight);
            if (!rendererResult) {
                showFatalError(rendererResult.error);
                return 3;
            }
        }

        POINT click{};
        if (window.consumeLeftClick(click)) {
            const float floorPlane = static_cast<float>(activeFloor) * kDemoFloorHeight;
            const auto picked = pickActiveFloor(
                click,
                window.clientWidth(),
                window.clientHeight(),
                floorPlane,
                camera);
            if (picked.has_value()) {
                focusTarget = Vec3{picked->x, picked->y + 1.0f, picked->z};
            }
        }

        RenderScene frameScene = baseScene;
        frameScene.activeFloor = activeFloor;
        frameScene.focusTarget = focusTarget;
        if (focusTarget.has_value()) {
            const float floorPlane = static_cast<float>(activeFloor) * kDemoFloorHeight;
            frameScene.items.push_back(makeBox(
                activeFloor,
                RenderCategory::Selection,
                {focusTarget->x, floorPlane + 0.06f, focusTarget->z},
                {0.8f, 0.12f, 0.8f},
                {1.0f, 0.68f, 0.08f, 0.90f}));
        }

        const hh::renderer::ComposedScene composed = composer.compose(
            frameScene,
            contextMode,
            wallMode,
            camera.worldPosition());
        rendererResult = renderer.render(composed, camera);
        if (!rendererResult) {
            showFatalError(rendererResult.error);
            return 4;
        }

        updateWindowTitle(window, activeFloor, contextMode, wallMode);
    }

    renderer.shutdown();
    return 0;
}
