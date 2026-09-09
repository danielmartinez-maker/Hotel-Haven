#include "win32/Win32Window.h"

namespace hh::renderer {

bool Win32Window::mousePosition(POINT& point) const noexcept {
    if (handle_ == nullptr) {
        return false;
    }
    POINT current{};
    if (GetCursorPos(&current) == FALSE || ScreenToClient(handle_, &current) == FALSE) {
        return false;
    }
    point = current;
    return true;
}

}  // namespace hh::renderer
