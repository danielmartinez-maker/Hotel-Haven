#include "win32/XInputMenuGamepad.h"

#include <windows.h>
#include <xinput.h>

namespace hh::frontend {

MenuInputFrame XInputMenuGamepad::poll() noexcept {
    XINPUT_STATE rawState{};
    MenuGamepadState state{};

    if (XInputGetState(0, &rawState) == ERROR_SUCCESS) {
        state.connected = true;
        const WORD buttons = rawState.Gamepad.wButtons;
        state.dpadUp = (buttons & XINPUT_GAMEPAD_DPAD_UP) != 0;
        state.dpadDown = (buttons & XINPUT_GAMEPAD_DPAD_DOWN) != 0;
        state.dpadLeft = (buttons & XINPUT_GAMEPAD_DPAD_LEFT) != 0;
        state.dpadRight = (buttons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0;
        state.a = (buttons & XINPUT_GAMEPAD_A) != 0;
        state.b = (buttons & XINPUT_GAMEPAD_B) != 0;
        state.leftStickX = rawState.Gamepad.sThumbLX;
        state.leftStickY = rawState.Gamepad.sThumbLY;
    }

    return mapper_.update(state);
}

}  // namespace hh::frontend
