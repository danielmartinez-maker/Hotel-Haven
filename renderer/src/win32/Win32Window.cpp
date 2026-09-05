#include "win32/Win32Window.h"

#include <windowsx.h>

#include <algorithm>
#include <string>

namespace hh::renderer {
namespace {

constexpr wchar_t kWindowClassName[] = L"HotelHavenRendererWindow";

bool validVirtualKey(int virtualKey) noexcept {
    return virtualKey >= 0 && virtualKey < 256;
}

}  // namespace

Win32Window::~Win32Window() {
    if (handle_ != nullptr && IsWindow(handle_) != FALSE) {
        DestroyWindow(handle_);
    }
}

bool Win32Window::create(
    HINSTANCE instance,
    int showCommand,
    std::uint32_t clientWidth,
    std::uint32_t clientHeight,
    std::string& error) {
    instance_ = instance;

    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = &Win32Window::windowProcedure;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.lpszClassName = kWindowClassName;

    if (RegisterClassExW(&windowClass) == 0) {
        const DWORD registrationError = GetLastError();
        if (registrationError != ERROR_CLASS_ALREADY_EXISTS) {
            error = "RegisterClassExW failed with Win32 error " +
                    std::to_string(registrationError);
            return false;
        }
    }

    RECT windowRectangle{
        0,
        0,
        static_cast<LONG>(clientWidth),
        static_cast<LONG>(clientHeight),
    };
    if (AdjustWindowRectEx(&windowRectangle, WS_OVERLAPPEDWINDOW, FALSE, 0) == FALSE) {
        error = "AdjustWindowRectEx failed with Win32 error " +
                std::to_string(GetLastError());
        return false;
    }

    const int windowWidth = windowRectangle.right - windowRectangle.left;
    const int windowHeight = windowRectangle.bottom - windowRectangle.top;
    handle_ = CreateWindowExW(
        0,
        kWindowClassName,
        L"Hotel Haven — Renderer Foundation C",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowWidth,
        windowHeight,
        nullptr,
        nullptr,
        instance,
        this);

    if (handle_ == nullptr) {
        error = "CreateWindowExW failed with Win32 error " +
                std::to_string(GetLastError());
        return false;
    }

    ShowWindow(handle_, showCommand);
    UpdateWindow(handle_);
    return true;
}

bool Win32Window::pumpMessages() {
    MSG message{};
    while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE) != FALSE) {
        if (message.message == WM_QUIT) {
            closed_ = true;
            continue;
        }
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return !closed_;
}

bool Win32Window::isKeyDown(int virtualKey) const noexcept {
    if (!validVirtualKey(virtualKey)) {
        return false;
    }
    return keyDown_[static_cast<std::size_t>(virtualKey)];
}

bool Win32Window::consumeKeyPressed(int virtualKey) noexcept {
    if (!validVirtualKey(virtualKey)) {
        return false;
    }
    const std::size_t index = static_cast<std::size_t>(virtualKey);
    const bool wasPressed = keyPressed_[index];
    keyPressed_[index] = false;
    return wasPressed;
}

int Win32Window::consumeMouseWheelDelta() noexcept {
    const int delta = mouseWheelDelta_;
    mouseWheelDelta_ = 0;
    return delta;
}

bool Win32Window::consumeLeftClick(POINT& point) noexcept {
    if (!leftClick_.has_value()) {
        return false;
    }
    point = *leftClick_;
    leftClick_.reset();
    return true;
}

bool Win32Window::consumeResize(
    std::uint32_t& width,
    std::uint32_t& height) noexcept {
    if (!resizePending_) {
        return false;
    }
    width = clientWidth_;
    height = clientHeight_;
    resizePending_ = false;
    return true;
}

void Win32Window::requestClose() noexcept {
    if (handle_ != nullptr) {
        PostMessageW(handle_, WM_CLOSE, 0, 0);
    }
}

void Win32Window::setTitle(std::wstring_view title) const {
    if (handle_ == nullptr) {
        return;
    }
    const std::wstring nullTerminated(title);
    SetWindowTextW(handle_, nullTerminated.c_str());
}

LRESULT CALLBACK Win32Window::windowProcedure(
    HWND window,
    UINT message,
    WPARAM wParam,
    LPARAM lParam) {
    Win32Window* self = reinterpret_cast<Win32Window*>(
        GetWindowLongPtrW(window, GWLP_USERDATA));

    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lParam);
        self = static_cast<Win32Window*>(create->lpCreateParams);
        self->handle_ = window;
        SetWindowLongPtrW(
            window,
            GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(self));
    }

    if (self != nullptr) {
        return self->handleMessage(message, wParam, lParam);
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

LRESULT Win32Window::handleMessage(UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CLOSE:
        DestroyWindow(handle_);
        return 0;

    case WM_DESTROY:
        handle_ = nullptr;
        closed_ = true;
        PostQuitMessage(0);
        return 0;

    case WM_SIZE:
        clientWidth_ = static_cast<std::uint32_t>(LOWORD(lParam));
        clientHeight_ = static_cast<std::uint32_t>(HIWORD(lParam));
        resizePending_ = true;
        return 0;

    case WM_KEYDOWN:
        if (wParam < keyDown_.size()) {
            const std::size_t index = static_cast<std::size_t>(wParam);
            const bool wasAlreadyDown = keyDown_[index];
            keyDown_[index] = true;
            if (!wasAlreadyDown) {
                keyPressed_[index] = true;
            }
        }
        return 0;

    case WM_KEYUP:
        if (wParam < keyDown_.size()) {
            keyDown_[static_cast<std::size_t>(wParam)] = false;
        }
        return 0;

    case WM_MOUSEWHEEL:
        mouseWheelDelta_ += GET_WHEEL_DELTA_WPARAM(wParam);
        return 0;

    case WM_LBUTTONDOWN:
        leftClick_ = POINT{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        return 0;

    default:
        return DefWindowProcW(handle_, message, wParam, lParam);
    }
}

}  // namespace hh::renderer
