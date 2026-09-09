#include "TestFramework.h"

#include "d2d/D2DUiRenderer.h"
#include "d2d/FontManager.h"

TEST_CASE("d2d renderer fails safely without d3d interop") {
    hh::frontend::D2DUiRenderer ui;
    const auto result = ui.initialize(nullptr, nullptr);
    EXPECT_FALSE(static_cast<bool>(result));
}

TEST_CASE("font manager always exposes display and interface fallback chains") {
    hh::frontend::FontManager fonts;
    EXPECT_FALSE(fonts.displayFallbackChain().empty());
    EXPECT_FALSE(fonts.interfaceFallbackChain().empty());
    EXPECT_EQ(fonts.displayFallbackChain().front(), std::wstring{L"Cormorant Garamond"});
    EXPECT_EQ(fonts.interfaceFallbackChain().front(), std::wstring{L"Inter"});
}
