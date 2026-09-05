#pragma once

#include <windows.h>

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace hh::renderer {

class Win32Window {
public:
    Win32Window() = default;
    ~Win32Window();

    Win32Window(const Win32Window&) = delete;
    Win32Window& operator=(const Win32Window&) = delete;

    [[nodiscard]] bool create(
        HINSTANCE instance,
        int showCommand,
        std::uint32_t clientWidth,
        std::uint32_t clientHeight,
        std::string& error);
    [[nodiscard]] bool pumpMessages();

    [[nodiscard]] HWND handle() const noexcept { return handle_; }
    [[nodiscard]] std::uint32_t clientWidth() const noexcept { return clientWidth_; }
    [[nodiscard]] std::uint32_t clientHeight() const noexcept { return clientHeight_; }

    [[nodiscard]] bool isKeyDown(int virtualKey) const noexcept;
    [[nodiscard]] bool consumeKeyPressed(int virtualKey) noexcept;
    [[nodiscard]] int consumeMouseWheelDelta() noexcept;
    [[nodiscard]] bool consumeLeftClick(POINT& point) noexcept;
    [[nodiscard]] bool consumeResize(std::uint32_t& width, std::uint32_t& height) noexcept;

    void requestClose() noexcept;
    void setTitle(std::wstring_view title) const;

private:
    static LRESULT CALLBACK windowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam);

    HWND handle_{};
    HINSTANCE instance_{};
    std::uint32_t clientWidth_{};
    std::uint32_t clientHeight_{};
    bool closed_{};
    bool resizePending_{};
    int mouseWheelDelta_{};
    std::optional<POINT> leftClick_;
    std::array<bool, 256> keyDown_{};
    std::array<bool, 256> keyPressed_{};
};

}  // namespace hh::renderer
