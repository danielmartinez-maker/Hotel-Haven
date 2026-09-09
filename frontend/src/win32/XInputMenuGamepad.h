#pragma once

#include "hh/frontend/MenuInput.h"

namespace hh::frontend {

class XInputMenuGamepad {
public:
    [[nodiscard]] MenuInputFrame poll() noexcept;

private:
    MenuGamepadMapper mapper_;
};

}  // namespace hh::frontend
