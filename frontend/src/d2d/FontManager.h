#pragma once

#include <dwrite.h>
#include <wrl/client.h>

#include <string>
#include <vector>

namespace hh::frontend {

class FontManager {
public:
    [[nodiscard]] const std::vector<std::wstring>& displayFallbackChain() const noexcept;
    [[nodiscard]] const std::vector<std::wstring>& interfaceFallbackChain() const noexcept;

    [[nodiscard]] Microsoft::WRL::ComPtr<IDWriteTextFormat> createDisplayFormat(
        IDWriteFactory* factory,
        float size,
        DWRITE_FONT_WEIGHT weight = DWRITE_FONT_WEIGHT_REGULAR) const;

    [[nodiscard]] Microsoft::WRL::ComPtr<IDWriteTextFormat> createInterfaceFormat(
        IDWriteFactory* factory,
        float size,
        DWRITE_FONT_WEIGHT weight = DWRITE_FONT_WEIGHT_REGULAR) const;

private:
    [[nodiscard]] Microsoft::WRL::ComPtr<IDWriteTextFormat> createFromChain(
        IDWriteFactory* factory,
        const std::vector<std::wstring>& chain,
        float size,
        DWRITE_FONT_WEIGHT weight) const;
};

}  // namespace hh::frontend
