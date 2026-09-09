#include "d2d/FontManager.h"

namespace hh::frontend {
namespace {
const std::vector<std::wstring> kDisplayFonts{L"Cormorant Garamond", L"Georgia", L"Times New Roman"};
const std::vector<std::wstring> kInterfaceFonts{L"Inter", L"Segoe UI", L"Arial"};
}

const std::vector<std::wstring>& FontManager::displayFallbackChain() const noexcept { return kDisplayFonts; }
const std::vector<std::wstring>& FontManager::interfaceFallbackChain() const noexcept { return kInterfaceFonts; }

Microsoft::WRL::ComPtr<IDWriteTextFormat> FontManager::createFromChain(
    IDWriteFactory* factory,
    const std::vector<std::wstring>& chain,
    float size,
    DWRITE_FONT_WEIGHT weight) const {
    Microsoft::WRL::ComPtr<IDWriteTextFormat> format;
    if (factory == nullptr) {
        return format;
    }
    for (const auto& family : chain) {
        format.Reset();
        const HRESULT result = factory->CreateTextFormat(
            family.c_str(), nullptr, weight, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, size, L"en-us", format.GetAddressOf());
        if (SUCCEEDED(result) && format != nullptr) {
            return format;
        }
    }
    return {};
}

Microsoft::WRL::ComPtr<IDWriteTextFormat> FontManager::createDisplayFormat(
    IDWriteFactory* factory, float size, DWRITE_FONT_WEIGHT weight) const {
    return createFromChain(factory, kDisplayFonts, size, weight);
}

Microsoft::WRL::ComPtr<IDWriteTextFormat> FontManager::createInterfaceFormat(
    IDWriteFactory* factory, float size, DWRITE_FONT_WEIGHT weight) const {
    return createFromChain(factory, kInterfaceFonts, size, weight);
}

}  // namespace hh::frontend
