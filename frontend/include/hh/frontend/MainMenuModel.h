#pragma once

#include <array>
#include <cstddef>

#include "hh/frontend/MainMenuCommands.h"

namespace hh::frontend {

class MainMenuModel {
public:
    explicit MainMenuModel(bool hasValidSave) noexcept;

    [[nodiscard]] static const std::array<MainMenuItem, 8>& orderedItems() noexcept;

    [[nodiscard]] MainMenuItem selected() const noexcept { return selected_; }
    [[nodiscard]] bool isEnabled(MainMenuItem item) const noexcept;
    [[nodiscard]] bool select(MainMenuItem item) noexcept;
    void setEnabled(MainMenuItem item, bool enabled) noexcept;
    void setContinueAvailable(bool available) noexcept;

    [[nodiscard]] MainMenuModal modal() const noexcept { return modal_; }
    void setModal(MainMenuModal modal) noexcept { modal_ = modal; }

private:
    [[nodiscard]] static std::size_t indexOf(MainMenuItem item) noexcept;
    [[nodiscard]] MainMenuItem firstEnabled() const noexcept;

    std::array<bool, 8> enabled_{};
    MainMenuItem selected_{MainMenuItem::NewHotel};
    MainMenuModal modal_{MainMenuModal::None};
};

}  // namespace hh::frontend
