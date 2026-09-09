#include <windows.h>

#include <chrono>
#include <filesystem>
#include <string>

#include "ShowcaseHotelScene.h"
#include "d2d/D2DUiRenderer.h"
#include "d3d11/D3D11Renderer.h"
#include "hh/frontend/MainMenuController.h"
#include "hh/frontend/MainMenuView.h"
#include "hh/frontend/MenuSceneController.h"
#include "hh/frontend/MenuTransitionDirector.h"
#include "hh/renderer/Camera.h"
#include "hh/renderer/FloorVisibility.h"
#include "hh/renderer/SceneComposer.h"
#include "win32/Win32Window.h"
#include "win32/XInputMenuGamepad.h"

namespace {

enum class QuitModalAction {
    None,
    Confirm,
    Cancel,
};

bool hasFlag(const wchar_t* flag) {
    const std::wstring commandLine = GetCommandLineW();
    return commandLine.find(flag) != std::wstring::npos;
}

hh::frontend::MenuPropertySummary demoProperty() {
    hh::frontend::MenuPropertySummary summary{};
    summary.hotelName = "The Beaumont";
    summary.city = "Paris";
    summary.country = "France";
    summary.starRating = 4;
    summary.currentDay = 184;
    summary.occupancy = 0.81F;
    summary.guestSatisfaction = 0.92F;
    summary.cashMinorUnits = 128000000;
    summary.currencyCode = "EUR";
    summary.roomCount = 247;
    return summary;
}

bool hitMenuItem(
    const hh::frontend::LayoutMetrics& layout,
    POINT point,
    hh::frontend::MainMenuItem& item) {
    float y = layout.navigationTop;
    for (const auto candidate : hh::frontend::MainMenuModel::orderedItems()) {
        if (candidate == hh::frontend::MainMenuItem::Settings) {
            y += 28.0F * layout.logicalScale;
        }
        const float height = 48.0F * layout.logicalScale;
        const float left = layout.navigationLeft - 12.0F * layout.logicalScale;
        const float right = layout.navigationLeft + layout.navigationWidth;
        if (static_cast<float>(point.x) >= left && static_cast<float>(point.x) <= right &&
            static_cast<float>(point.y) >= y && static_cast<float>(point.y) <= y + height) {
            item = candidate;
            return true;
        }
        y += 55.0F * layout.logicalScale;
    }
    return false;
}

QuitModalAction hitQuitModalAction(
    const hh::frontend::LayoutMetrics& layout,
    float width,
    float height,
    POINT point) noexcept {
    const float scale = layout.logicalScale;
    const float modalWidth = 470.0F * scale;
    const float modalHeight = 190.0F * scale;
    const float left = (width - modalWidth) * 0.5F;
    const float top = (height - modalHeight) * 0.5F;
    const float buttonTop = top + 102.0F * scale;
    const float buttonBottom = top + 170.0F * scale;
    const float x = static_cast<float>(point.x);
    const float y = static_cast<float>(point.y);

    if (x < left + 24.0F * scale || x > left + modalWidth - 24.0F * scale ||
        y < buttonTop || y > buttonBottom) {
        return QuitModalAction::None;
    }

    return x < left + modalWidth * 0.5F
        ? QuitModalAction::Confirm
        : QuitModalAction::Cancel;
}

bool hitSettingsToggle(
    const hh::frontend::LayoutMetrics& layout,
    float width,
    float height,
    POINT point) noexcept {
    const float scale = layout.logicalScale;
    const float panelWidth = 620.0F * scale;
    const float panelHeight = 310.0F * scale;
    const float left = (width - panelWidth) * 0.5F;
    const float top = (height - panelHeight) * 0.5F;
    const float x = static_cast<float>(point.x);
    const float y = static_cast<float>(point.y);
    return x >= left + 32.0F * scale && x <= left + panelWidth - 32.0F * scale &&
           y >= top + 98.0F * scale && y <= top + 168.0F * scale;
}

void executeCommand(hh::frontend::MainMenuCommand command, hh::renderer::Win32Window& window) {
    using hh::frontend::MainMenuCommand;
    switch (command) {
        case MainMenuCommand::ExitApplication:
            window.requestClose();
            break;
        case MainMenuCommand::ContinueLatest:
            window.setTitle(L"Hotel Haven — Continue transition fixture");
            break;
        case MainMenuCommand::StartNewHotel:
            window.setTitle(L"Hotel Haven — New Hotel flow fixture");
            break;
        case MainMenuCommand::OpenLoadHotel:
            window.setTitle(L"Hotel Haven — Load Hotel fixture");
            break;
        case MainMenuCommand::OpenScenarios:
            window.setTitle(L"Hotel Haven — Scenarios fixture");
            break;
        case MainMenuCommand::OpenSandbox:
            window.setTitle(L"Hotel Haven — Sandbox fixture");
            break;
        case MainMenuCommand::OpenSettings:
        case MainMenuCommand::OpenCredits:
        case MainMenuCommand::None:
            break;
    }
}

void toggleReducedMotion(
    bool& reducedMotion,
    hh::frontend::MenuSceneController& sceneController,
    hh::frontend::MenuTransitionDirector& transitions,
    const hh::frontend::MainMenuModel& model,
    hh::renderer::RenderScene& scene) {
    reducedMotion = !reducedMotion;
    sceneController.setReducedMotion(reducedMotion);
    transitions.retarget(model.selected(), reducedMotion);
    if (reducedMotion) {
        scene = hh::frontend::ShowcaseHotelScene::build();
    }
}

}  // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand) {
    constexpr std::uint32_t kWidth = 1600;
    constexpr std::uint32_t kHeight = 900;

    hh::renderer::Win32Window window;
    std::string error;
    if (!window.create(instance, showCommand, kWidth, kHeight, error)) {
        MessageBoxA(nullptr, error.c_str(), "Hotel Haven", MB_OK | MB_ICONERROR);
        return 1;
    }
    window.setTitle(L"Hotel Haven — Living Hotel Main Menu v0.1");

    const std::filesystem::path shaderPath =
        std::filesystem::path(L"shaders") / L"InstancedBox.hlsl";

    hh::renderer::D3D11Renderer renderer;
    auto rendererResult = renderer.initialize(window.handle(), kWidth, kHeight, shaderPath);
    if (!rendererResult) {
        MessageBoxA(nullptr, rendererResult.error.c_str(), "Hotel Haven Renderer", MB_OK | MB_ICONERROR);
        return 2;
    }

    const bool validSave = hasFlag(L"--valid-save");
    bool reducedMotion = hasFlag(L"--reduced-motion");
    const auto property = demoProperty();

    hh::frontend::MainMenuModel model(validSave);
    hh::frontend::MainMenuController controller(model);
    hh::frontend::MainMenuView view;
    hh::frontend::MenuTransitionDirector transitions;
    hh::frontend::MenuSceneController sceneController;
    hh::frontend::XInputMenuGamepad gamepad;
    sceneController.setReducedMotion(reducedMotion);
    transitions.retarget(model.selected(), reducedMotion);

    hh::frontend::D2DUiRenderer ui;
    auto uiResult = ui.initialize(renderer.device(), renderer.swapChain());
    if (!uiResult) {
        MessageBoxA(nullptr, uiResult.error.c_str(), "Hotel Haven UI", MB_OK | MB_ICONERROR);
        return 3;
    }

    hh::renderer::RenderScene scene = hh::frontend::ShowcaseHotelScene::build();
    hh::renderer::SceneComposer composer;
    hh::renderer::OrthoCamera camera;
    camera.setTarget({0.0F, 3.5F, 0.0F});
    camera.setPitchDegrees(55.0F);
    camera.setYawDegrees(-25.0F);
    camera.setOrthoHeight(42.0F);

    auto previous = std::chrono::steady_clock::now();
    const auto started = previous;

    while (window.pumpMessages()) {
        const auto now = std::chrono::steady_clock::now();
        const float deltaSeconds = std::chrono::duration<float>(now - previous).count();
        const float elapsedSeconds = std::chrono::duration<float>(now - started).count();
        previous = now;

        const hh::frontend::MenuInputFrame gamepadInput = gamepad.poll();
        if (window.consumeKeyPressed(VK_UP) || window.consumeKeyPressed('W') ||
            gamepadInput.navigationDelta < 0) {
            controller.navigate(-1);
            transitions.retarget(model.selected(), reducedMotion);
        }
        if (window.consumeKeyPressed(VK_DOWN) || window.consumeKeyPressed('S') ||
            gamepadInput.navigationDelta > 0) {
            controller.navigate(1);
            transitions.retarget(model.selected(), reducedMotion);
        }

        const bool activatePressed =
            window.consumeKeyPressed(VK_RETURN) || window.consumeKeyPressed(VK_SPACE) ||
            gamepadInput.activate;
        if (activatePressed) {
            if (model.modal() == hh::frontend::MainMenuModal::QuitConfirm) {
                executeCommand(controller.confirmQuit(), window);
            } else if (model.panel() == hh::frontend::MainMenuPanel::Settings) {
                toggleReducedMotion(reducedMotion, sceneController, transitions, model, scene);
            } else if (model.panel() == hh::frontend::MainMenuPanel::None) {
                executeCommand(controller.activate(), window);
            }
        }

        const bool cancelPressed = window.consumeKeyPressed(VK_ESCAPE) || gamepadInput.cancel;
        if (cancelPressed) {
            if (!controller.cancel()) {
                window.setTitle(L"Hotel Haven — Living Hotel Main Menu v0.1");
            }
        }

        const auto layout = view.layout(
            static_cast<float>(window.clientWidth()),
            static_cast<float>(window.clientHeight()), 1.0F);
        POINT pointer{};
        hh::frontend::MainMenuItem hovered{};
        if (model.modal() == hh::frontend::MainMenuModal::None &&
            model.panel() == hh::frontend::MainMenuPanel::None &&
            window.mousePosition(pointer) &&
            hitMenuItem(layout, pointer, hovered) &&
            model.isEnabled(hovered)) {
            if (controller.hover(hovered)) {
                transitions.retarget(model.selected(), reducedMotion);
            }
        }

        POINT click{};
        if (window.consumeLeftClick(click)) {
            if (model.modal() == hh::frontend::MainMenuModal::QuitConfirm) {
                const auto action = hitQuitModalAction(
                    layout,
                    static_cast<float>(window.clientWidth()),
                    static_cast<float>(window.clientHeight()),
                    click);
                if (action == QuitModalAction::Confirm) {
                    executeCommand(controller.confirmQuit(), window);
                } else if (action == QuitModalAction::Cancel) {
                    static_cast<void>(controller.cancel());
                }
            } else if (model.panel() == hh::frontend::MainMenuPanel::Settings &&
                       hitSettingsToggle(
                           layout,
                           static_cast<float>(window.clientWidth()),
                           static_cast<float>(window.clientHeight()),
                           click)) {
                toggleReducedMotion(reducedMotion, sceneController, transitions, model, scene);
            } else if (model.panel() == hh::frontend::MainMenuPanel::None &&
                       hitMenuItem(layout, click, hovered) && model.isEnabled(hovered)) {
                static_cast<void>(controller.hover(hovered));
                transitions.retarget(model.selected(), reducedMotion);
                executeCommand(controller.activate(), window);
            }
        }

        std::uint32_t resizeWidth{};
        std::uint32_t resizeHeight{};
        if (window.consumeResize(resizeWidth, resizeHeight) && resizeWidth > 0 && resizeHeight > 0) {
            ui.discardDeviceResources();
            rendererResult = renderer.resize(resizeWidth, resizeHeight);
            if (!rendererResult) {
                break;
            }
            uiResult = ui.resize(renderer.swapChain());
            if (!uiResult) {
                break;
            }
        }

        if (window.clientWidth() == 0 || window.clientHeight() == 0) {
            Sleep(16);
            continue;
        }

        const auto transition = transitions.update(deltaSeconds);
        const auto pose = sceneController.cameraPose(elapsedSeconds, transition);
        camera.setAspectRatio(
            static_cast<float>(window.clientWidth()) /
            static_cast<float>(window.clientHeight()));
        camera.setYawDegrees(-25.0F + pose.idleYawDegrees + pose.yawOffsetDegrees);
        camera.setOrthoHeight(42.0F * pose.idleZoomScale * pose.zoomScale);
        camera.setTarget({pose.targetXOffset, 3.5F, pose.targetZOffset});

        if (!reducedMotion) {
            hh::frontend::ShowcaseHotelScene::updateAmbient(scene, elapsedSeconds);
        }

        const auto composed = composer.compose(
            scene,
            hh::renderer::FloorContextMode::AdjacentContext,
            hh::renderer::WallRenderMode::Cutaway,
            camera.worldPosition());

        rendererResult = renderer.renderWorld(composed, camera);
        if (!rendererResult) {
            break;
        }

        hh::frontend::UiFrameState frame{};
        frame.width = static_cast<float>(window.clientWidth());
        frame.height = static_cast<float>(window.clientHeight());
        frame.uiScale = 1.0F;
        frame.property = validSave ? &property : nullptr;
        frame.reducedMotion = reducedMotion;
        uiResult = ui.draw(model, view, frame);
        if (!uiResult && uiResult.recreateTarget) {
            uiResult = ui.resize(renderer.swapChain());
        }
        if (!uiResult) {
            break;
        }

        rendererResult = renderer.present();
        if (!rendererResult) {
            break;
        }
    }

    renderer.shutdown();
    return 0;
}
