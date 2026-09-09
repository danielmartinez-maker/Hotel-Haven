#include "d2d/UiPrimitiveRenderer.h"

namespace hh::frontend {

void UiPrimitiveRenderer::fillRect(const D2D1_RECT_F& rect, const D2D1_COLOR_F& color) {
    if (context_ == nullptr) {
        return;
    }
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
    if (SUCCEEDED(context_->CreateSolidColorBrush(color, brush.GetAddressOf()))) {
        context_->FillRectangle(rect, brush.Get());
    }
}

void UiPrimitiveRenderer::strokeRect(const D2D1_RECT_F& rect, const D2D1_COLOR_F& color, float width) {
    if (context_ == nullptr) {
        return;
    }
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
    if (SUCCEEDED(context_->CreateSolidColorBrush(color, brush.GetAddressOf()))) {
        context_->DrawRectangle(rect, brush.Get(), width);
    }
}

}  // namespace hh::frontend
