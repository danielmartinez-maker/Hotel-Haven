#pragma once

#include <cstdint>

namespace hh::frontend {

struct MenuInputFrame {
    int navigationDelta{};
    bool activate{};
    bool cancel{};
};

struct MenuGamepadState {
    bool connected{};
    bool dpadUp{};
    bool dpadDown{};
    bool dpadLeft{};
    bool dpadRight{};
    bool a{};
    bool b{};
    std::int16_t leftStickX{};
    std::int16_t leftStickY{};
};

class MenuGamepadMapper {
public:
    [[nodiscard]] MenuInputFrame update(const MenuGamepadState& state) noexcept;

private:
    bool previousPrevious_{};
    bool previousNext_{};
    bool previousActivate_{};
    bool previousCancel_{};
};

}  // namespace hh::frontend
