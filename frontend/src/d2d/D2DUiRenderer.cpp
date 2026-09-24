#include "d2d/D2DUiRenderer.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <sstream>

#include "d2d/FontManager.h"
#include "d2d/UiPrimitiveRenderer.h"

namespace hh::frontend {
namespace {

constexpr D2D1_COLOR_F kIvory{0.949F, 0.925F, 0.882F, 1.0F};
constexpr D2D1_COLOR_F kMuted{0.722F, 0.690F, 0.643F, 1.0F};
constexpr D2D1_COLOR_F kBrass{0.725F, 0.592F, 0.357F, 1.0F};
constexpr D2D1_COLOR_F kPanel{0.082F, 0.078F, 0.071F, 0.78F};
constexpr D2D1_COLOR_F kSelected{0.725F, 0.592F, 0.357F, 0.34F};
constexpr D2D1_COLOR_F kDisabled{0.949F, 0.925F, 0.882F, 0.30F};

std::wstring widen(const std::string& value) {
    return std::wstring(value.begin(), value.end());
}

std::wstring labelFor(MainMenuItem item) {
    switch (item) {
        case MainMenuItem::Continue: return L"CONTINUE";
        case MainMenuItem::NewHotel: return L"NEW HOTEL";
        case MainMenuItem::LoadHotel: return L"LOAD HOTEL";
        case MainMenuItem::Scenarios: return L"SCENARIOS";
        case MainMenuItem::Sandbox: return L"SANDBOX";
        case MainMenuItem::Settings: return L"SETTINGS";
        case MainMenuItem::Credits: return L"CREDITS";
        case MainMenuItem::Quit: return L"QUIT";
    }
    return L"";
}

std::string hresultMessage(const char* operation, HRESULT result) {
    std::ostringstream stream;
    stream << operation << " failed (HRESULT 0x" << std::hex << std::uppercase
           << static_cast<unsigned long>(result) << ')';
    return stream.str();
}

}  // namespace

UiRendererResult D2DUiRenderer::initialize(ID3D11Device* device, IDXGISwapChain* swapChain) {
    discardDeviceResources();
    factory_.Reset();
    d2dDevice_.Reset();
    context_.Reset();
    writeFactory_.Reset();

    if (device == nullptr || swapChain == nullptr) {
        return UiRendererResult::failure("D2D initialize requires D3D11 device and swap chain");
    }

    D2D1_FACTORY_OPTIONS options{};
    HRESULT result = D2D1CreateFactory(
        D2D1_FACTORY_TYPE_SINGLE_THREADED,
        __uuidof(ID2D1Factory1),
        &options,
        reinterpret_cast<void**>(factory_.GetAddressOf()));
    if (FAILED(result)) {
        return UiRendererResult::failure(hresultMessage("D2D1CreateFactory", result));
    }

    Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
    result = device->QueryInterface(IID_PPV_ARGS(dxgiDevice.GetAddressOf()));
    if (FAILED(result)) {
        return UiRendererResult::failure(hresultMessage("ID3D11Device::QueryInterface(IDXGIDevice)", result));
    }

    result = factory_->CreateDevice(dxgiDevice.Get(), d2dDevice_.GetAddressOf());
    if (FAILED(result)) {
        return UiRendererResult::failure(hresultMessage("ID2D1Factory1::CreateDevice", result));
    }

    result = d2dDevice_->CreateDeviceContext(
        D2D1_DEVICE_CONTEXT_OPTIONS_NONE, context_.GetAddressOf());
    if (FAILED(result)) {
        return UiRendererResult::failure(hresultMessage("ID2D1Device::CreateDeviceContext", result));
    }

    result = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(writeFactory_.GetAddressOf()));
    if (FAILED(result)) {
        return UiRendererResult::failure(hresultMessage("DWriteCreateFactory", result));
    }

    return createTargetBitmap(swapChain);
}

UiRendererResult D2DUiRenderer::createTargetBitmap(IDXGISwapChain* swapChain) {
    if (context_ == nullptr || swapChain == nullptr) {
        return UiRendererResult::failure("D2D target creation requires initialized context and swap chain");
    }

    target_.Reset();
    Microsoft::WRL::ComPtr<IDXGISurface> surface;
    const HRESULT surfaceResult = swapChain->GetBuffer(0, IID_PPV_ARGS(surface.GetAddressOf()));
    if (FAILED(surfaceResult)) {
        return UiRendererResult::failure(hresultMessage("IDXGISwapChain::GetBuffer(IDXGISurface)", surfaceResult));
    }

    const D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat(DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

    const HRESULT bitmapResult = context_->CreateBitmapFromDxgiSurface(
        surface.Get(), &bitmapProperties, target_.GetAddressOf());
    if (FAILED(bitmapResult)) {
        return UiRendererResult::failure(hresultMessage("CreateBitmapFromDxgiSurface", bitmapResult));
    }
    context_->SetTarget(target_.Get());
    return UiRendererResult::success();
}

UiRendererResult D2DUiRenderer::resize(IDXGISwapChain* swapChain) {
    if (context_ == nullptr) {
        return UiRendererResult::failure("D2D resize called before initialization");
    }
    context_->SetTarget(nullptr);
    target_.Reset();
    return createTargetBitmap(swapChain);
}

void D2DUiRenderer::discardDeviceResources() noexcept {
    if (context_ != nullptr) {
        context_->SetTarget(nullptr);
    }
    target_.Reset();
}

void D2DUiRenderer::drawText(
    const std::wstring& text,
    const D2D1_RECT_F& rect,
    IDWriteTextFormat* format,
    const D2D1_COLOR_F& color) {
    if (context_ == nullptr || format == nullptr) {
        return;
    }
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
    if (FAILED(context_->CreateSolidColorBrush(color, brush.GetAddressOf()))) {
        return;
    }
    context_->DrawText(text.c_str(), static_cast<UINT32>(text.size()), format, rect, brush.Get());
}

UiRendererResult D2DUiRenderer::draw(
    const MainMenuModel& model,
    const MainMenuView& view,
    const UiFrameState& frameState) {
    if (context_ == nullptr || target_ == nullptr || writeFactory_ == nullptr) {
        return UiRendererResult::failure("D2D draw called before initialization");
    }

    const LayoutMetrics layout =
        view.layout(frameState.width, frameState.height, frameState.uiScale);
    const float ui = layout.uiContentScale;
    FontManager fonts;
    auto brandFormat = fonts.createDisplayFormat(writeFactory_.Get(), 42.0F * ui, DWRITE_FONT_WEIGHT_SEMI_BOLD);
    auto propertyFormat = fonts.createDisplayFormat(writeFactory_.Get(), 30.0F * ui, DWRITE_FONT_WEIGHT_SEMI_BOLD);
    auto menuFormat = fonts.createInterfaceFormat(writeFactory_.Get(), 22.0F * ui, DWRITE_FONT_WEIGHT_SEMI_BOLD);
    auto smallFormat = fonts.createInterfaceFormat(writeFactory_.Get(), 14.0F * ui);
    if (brandFormat == nullptr || propertyFormat == nullptr || menuFormat == nullptr || smallFormat == nullptr) {
        return UiRendererResult::failure("DirectWrite font creation failed for all configured fallbacks");
    }

    context_->BeginDraw();
    context_->SetTransform(D2D1::Matrix3x2F::Identity());
    UiPrimitiveRenderer primitives(context_.Get());

    const float panelLeft = std::max(0.0F, layout.safeZoneLeft + 38.0F * ui);
    primitives.fillRect(
        D2D1::RectF(panelLeft, 0.0F,
                    layout.navigationLeft + layout.navigationWidth + 34.0F * ui,
                    frameState.height),
        kPanel);

    drawText(L"HOTEL HAVEN",
             D2D1::RectF(layout.navigationLeft, 64.0F * ui,
                         layout.navigationLeft + 430.0F * ui, 125.0F * ui),
             brandFormat.Get(), kIvory);
    drawText(L"BUILD · MANAGE · BELONG",
             D2D1::RectF(layout.navigationLeft, 126.0F * ui,
                         layout.navigationLeft + 430.0F * ui, 155.0F * ui),
             smallFormat.Get(), kMuted);

    float y = layout.navigationTop;
    for (const MainMenuItem item : MainMenuModel::orderedItems()) {
        if (item == MainMenuItem::Settings) {
            y += 28.0F * ui;
        }
        const float itemHeight = 48.0F * ui;
        const D2D1_RECT_F rect = D2D1::RectF(
            layout.navigationLeft - 12.0F * ui,
            y,
            layout.navigationLeft + layout.navigationWidth,
            y + itemHeight);
        const bool enabled = model.isEnabled(item);
        const bool selected = model.selected() == item;
        if (selected && enabled) {
            primitives.fillRect(rect, kSelected);
            primitives.strokeRect(rect, kBrass, 1.0F * ui);
        }
        const float shift = selected && enabled ? 10.0F * ui : 0.0F;
        if (selected && enabled) {
            primitives.fillRect(
                D2D1::RectF(rect.left, rect.top, rect.left + 4.0F * ui, rect.bottom),
                kBrass);
        }
        std::wstring itemLabel = labelFor(item);
        if (!enabled && item == MainMenuItem::Continue) {
            itemLabel += L"  ·  NO SAVE";
        }
        drawText(itemLabel,
                 D2D1::RectF(layout.navigationLeft + shift, y + 9.0F * ui,
                             layout.navigationLeft + layout.navigationWidth, y + itemHeight),
                 menuFormat.Get(), enabled ? kIvory : kDisabled);
        y += 55.0F * ui;
    }

    drawText(widen(frameState.version),
             D2D1::RectF(layout.versionLeft, layout.versionBottom - 28.0F * ui,
                         layout.versionLeft + 250.0F * ui, layout.versionBottom),
             smallFormat.Get(), kMuted);
    const float inputHintTop =
        std::max(164.0F * ui, layout.navigationTop - 58.0F * ui);
    drawText(L"ENTER / A  SELECT    ESC / B  BACK",
             D2D1::RectF(layout.navigationLeft,
                         inputHintTop,
                         layout.navigationLeft + 430.0F * ui,
                         inputHintTop + 30.0F * ui),
             smallFormat.Get(), kMuted);

    if (frameState.property != nullptr) {
        const auto card = view.formatProperty(*frameState.property);
        const float left = layout.propertyCardLeft;
        const float top = layout.propertyCardTop;
        const float right = left + layout.propertyCardWidth;
        primitives.fillRect(D2D1::RectF(left - 24.0F * ui,
                                        top - 22.0F * ui,
                                        right + 24.0F * ui,
                                        top + 390.0F * ui), kPanel);
        drawText(widen(card.hotelName), D2D1::RectF(left, top, right, top + 48.0F * ui), propertyFormat.Get(), kIvory);
        drawText(widen(card.location), D2D1::RectF(left, top + 48.0F * ui, right, top + 78.0F * ui), smallFormat.Get(), kMuted);
        std::wstring stars;
        for (int i = 0; i < 5; ++i) {
            stars += i < card.visualStars ? L"★" : L"☆";
        }
        drawText(stars, D2D1::RectF(left, top + 80.0F * ui, right, top + 112.0F * ui), menuFormat.Get(), kBrass);

        const std::array<std::pair<std::wstring, std::string>, 5> stats{{
            {L"DAY", card.day}, {L"OCCUPANCY", card.occupancy},
            {L"GUEST SATISFACTION", card.satisfaction}, {L"CASH", card.cash},
            {L"ROOMS", card.rooms}
        }};
        float sy = top + 145.0F * ui;
        for (const auto& [label, value] : stats) {
            drawText(label, D2D1::RectF(left, sy, left + 190.0F * ui, sy + 26.0F * ui), smallFormat.Get(), kMuted);
            drawText(widen(value), D2D1::RectF(left + 180.0F * ui, sy, right, sy + 26.0F * ui), smallFormat.Get(), kIvory);
            sy += 43.0F * ui;
        }
    }

    if (model.panel() != MainMenuPanel::None) {
        const float panelWidth = 620.0F * ui;
        const float panelHeight = 310.0F * ui;
        const float left = (frameState.width - panelWidth) * 0.5F;
        const float top = (frameState.height - panelHeight) * 0.5F;
        const float right = left + panelWidth;
        const float bottom = top + panelHeight;
        primitives.fillRect(D2D1::RectF(left, top, right, bottom),
                            D2D1::ColorF(0.05F, 0.05F, 0.045F, 0.96F));
        primitives.strokeRect(D2D1::RectF(left, top, right, bottom), kBrass,
                              1.0F * ui);

        if (model.panel() == MainMenuPanel::Settings) {
            drawText(L"SETTINGS",
                     D2D1::RectF(left + 38.0F * ui,
                                 top + 30.0F * ui,
                                 right - 38.0F * ui,
                                 top + 78.0F * ui),
                     propertyFormat.Get(), kIvory);

            const float rowLeft = left + 30.0F * ui;
            const float rowRight = right - 30.0F * ui;
            const float rowHeight = 52.0F * ui;
            const float scaleRowTop = top + 98.0F * ui;
            const float motionRowTop = top + 158.0F * ui;
            const auto drawSettingRow =
                [&](MainMenuSettingsItem item, float rowTop,
                    const wchar_t* label, const std::wstring& value) {
                    const bool selected = model.settingsSelection() == item;
                    const D2D1_RECT_F rowRect =
                        D2D1::RectF(rowLeft, rowTop, rowRight,
                                    rowTop + rowHeight);
                    if (selected) {
                        primitives.fillRect(rowRect, kSelected);
                        primitives.strokeRect(rowRect, kBrass,
                                              std::max(1.0F, ui));
                        primitives.fillRect(
                            D2D1::RectF(rowRect.left, rowRect.top,
                                        rowRect.left + 4.0F * ui,
                                        rowRect.bottom),
                            kBrass);
                    }
                    drawText(label,
                             D2D1::RectF(rowLeft + 14.0F * ui,
                                         rowTop + 12.0F * ui,
                                         left + 350.0F * ui,
                                         rowTop + 42.0F * ui),
                             menuFormat.Get(), selected ? kIvory : kMuted);
                    drawText(value,
                             D2D1::RectF(left + 375.0F * ui,
                                         rowTop + 12.0F * ui,
                                         rowRight - 12.0F * ui,
                                         rowTop + 42.0F * ui),
                             menuFormat.Get(), kBrass);
                };

            const int scalePercent =
                static_cast<int>(std::lround(frameState.uiScale * 100.0F));
            drawSettingRow(MainMenuSettingsItem::UiScale, scaleRowTop,
                           L"UI SCALE",
                           L"[ " + std::to_wstring(scalePercent) + L"% ]");
            drawSettingRow(MainMenuSettingsItem::ReducedMotion, motionRowTop,
                           L"REDUCED MOTION",
                           frameState.reducedMotion ? L"[ ON ]" : L"[ OFF ]");

            drawText(L"UP / DOWN TO SELECT    ENTER / A TO CHANGE    ESC / B TO CLOSE",
                     D2D1::RectF(left + 38.0F * ui,
                                 bottom - 50.0F * ui,
                                 right - 38.0F * ui,
                                 bottom - 20.0F * ui),
                     smallFormat.Get(), kMuted);
        } else {
            drawText(L"CREDITS",
                     D2D1::RectF(left + 38.0F * ui,
                                 top + 30.0F * ui,
                                 right - 38.0F * ui,
                                 top + 78.0F * ui),
                     propertyFormat.Get(), kIvory);
            drawText(L"HOTEL HAVEN",
                     D2D1::RectF(left + 38.0F * ui,
                                 top + 112.0F * ui,
                                 right - 38.0F * ui,
                                 top + 150.0F * ui),
                     menuFormat.Get(), kIvory);
            drawText(L"Created by Daniel Martinez",
                     D2D1::RectF(left + 38.0F * ui,
                                 top + 158.0F * ui,
                                 right - 38.0F * ui,
                                 top + 190.0F * ui),
                     smallFormat.Get(), kMuted);
            drawText(L"ESC / B TO CLOSE",
                     D2D1::RectF(left + 38.0F * ui,
                                 bottom - 58.0F * ui,
                                 right - 38.0F * ui,
                                 bottom - 25.0F * ui),
                     smallFormat.Get(), kMuted);
        }
    }

    if (model.modal() == MainMenuModal::QuitConfirm) {
        const float w = 470.0F * ui;
        const float h = 190.0F * ui;
        const float left = (frameState.width - w) * 0.5F;
        const float top = (frameState.height - h) * 0.5F;
        primitives.fillRect(D2D1::RectF(left, top, left + w, top + h), D2D1::ColorF(0.05F, 0.05F, 0.045F, 0.96F));
        primitives.strokeRect(D2D1::RectF(left, top, left + w, top + h),
                              kBrass, std::max(1.0F, ui));
        drawText(L"Exit Hotel Haven?",
                 D2D1::RectF(left + 34.0F * ui, top + 30.0F * ui,
                             left + w - 30.0F * ui, top + 78.0F * ui),
                 propertyFormat.Get(), kIvory);
        drawText(L"[ EXIT ]    [ CANCEL ]",
                 D2D1::RectF(left + 34.0F * ui, top + 116.0F * ui,
                             left + w - 30.0F * ui, top + 155.0F * ui),
                 menuFormat.Get(), kIvory);
    }

    const HRESULT result = context_->EndDraw();
    if (result == D2DERR_RECREATE_TARGET) {
        discardDeviceResources();
        return UiRendererResult::failure("Direct2D target must be recreated", true);
    }
    if (FAILED(result)) {
        return UiRendererResult::failure(hresultMessage("ID2D1DeviceContext::EndDraw", result));
    }
    return UiRendererResult::success();
}

}  // namespace hh::frontend
