#pragma once

#include "hh/frontend/MainMenuModel.h"

namespace hh::frontend {

class MainMenuController {
public:
    explicit MainMenuController(MainMenuModel& model) noexcept : model_(model) {}

    void navigate(int delta) noexcept;
    [[nodiscard]] MainMenuCommand activate() noexcept;
    [[nodiscard]] MainMenuCommand confirmQuit() noexcept;
    [[nodiscard]] bool cancel() noexcept;
    [[nodiscard]] bool hover(MainMenuItem item) noexcept;

private:
    [[nodiscard]] static MainMenuCommand commandFor(MainMenuItem item) noexcept;

    MainMenuModel& model_;
};

}  // namespace hh::frontend
