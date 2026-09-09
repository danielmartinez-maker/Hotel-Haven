#pragma once

#include <d2d1_1.h>
#include <dwrite.h>
#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>

#include <string>

#include "hh/frontend/MainMenuModel.h"
#include "hh/frontend/MainMenuView.h"

namespace hh::frontend {

struct UiRendererResult {
    bool succeeded{true};
    bool recreateTarget{false};
    std::string error;

    [[nodiscard]] explicit operator bool() const noexcept { return succeeded; }
    [[nodiscard]] static UiRendererResult success() { return {}; }
    [[nodiscard]] static UiRendererResult failure(std::string message, bool recreate = false) {
        return UiRendererResult{false, recreate, std::move(message)};
    }
};

struct UiFrameState {
    float width{1920.0F};
    float height{1080.0F};
    float uiScale{1.0F};
    const MenuPropertySummary* property{};
    std::string version{"v0.1.0-alpha"};
};

class D2DUiRenderer {
public:
    D2DUiRenderer() = default;
    ~D2DUiRenderer() = default;

    [[nodiscard]] UiRendererResult initialize(ID3D11Device* device, IDXGISwapChain* swapChain);
    [[nodiscard]] UiRendererResult resize(IDXGISwapChain* swapChain);
    [[nodiscard]] UiRendererResult draw(
        const MainMenuModel& model,
        const MainMenuView& view,
        const UiFrameState& frameState);
    void discardDeviceResources() noexcept;

private:
    [[nodiscard]] UiRendererResult createTargetBitmap(IDXGISwapChain* swapChain);
    void drawText(const std::wstring& text, const D2D1_RECT_F& rect, IDWriteTextFormat* format,
                  const D2D1_COLOR_F& color);

    Microsoft::WRL::ComPtr<ID2D1Factory1> factory_;
    Microsoft::WRL::ComPtr<ID2D1Device> d2dDevice_;
    Microsoft::WRL::ComPtr<ID2D1DeviceContext> context_;
    Microsoft::WRL::ComPtr<ID2D1Bitmap1> target_;
    Microsoft::WRL::ComPtr<IDWriteFactory> writeFactory_;
};

}  // namespace hh::frontend
