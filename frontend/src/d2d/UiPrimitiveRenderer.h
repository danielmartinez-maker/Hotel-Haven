#pragma once

#include <d2d1_1.h>
#include <wrl/client.h>

namespace hh::frontend {

class UiPrimitiveRenderer {
public:
    explicit UiPrimitiveRenderer(ID2D1DeviceContext* context = nullptr) noexcept : context_(context) {}
    void setContext(ID2D1DeviceContext* context) noexcept { context_ = context; }

    void fillRect(const D2D1_RECT_F& rect, const D2D1_COLOR_F& color);
    void strokeRect(const D2D1_RECT_F& rect, const D2D1_COLOR_F& color, float width = 1.0F);

private:
    ID2D1DeviceContext* context_{};
};

}  // namespace hh::frontend
