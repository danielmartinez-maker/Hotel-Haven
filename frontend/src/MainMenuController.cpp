#include "hh/frontend/MainMenuController.h"

#include <cstddef>

namespace hh::frontend {

void MainMenuController::navigate(int delta) noexcept {
    if (delta == 0 || model_.modal() != MainMenuModal::None) {
        return;
    }

    const auto& items = MainMenuModel::orderedItems();
    std::size_t current = 0;
    for (std::size_t i = 0; i < items.size(); ++i) {
        if (items[i] == model_.selected()) {
            current = i;
            break;
        }
    }

    const int direction = delta < 0 ? -1 : 1;
    for (std::size_t attempts = 0; attempts < items.size(); ++attempts) {
        if (direction < 0) {
            current = current == 0 ? items.size() - 1 : current - 1;
        } else {
            current = (current + 1) % items.size();
        }
        if (model_.isEnabled(items[current])) {
            model_.select(items[current]);
            return;
        }
    }
}

MainMenuCommand MainMenuController::activate() noexcept {
    if (model_.modal() != MainMenuModal::None || !model_.isEnabled(model_.selected())) {
        return MainMenuCommand::None;
    }
    if (model_.selected() == MainMenuItem::Quit) {
        model_.setModal(MainMenuModal::QuitConfirm);
        return MainMenuCommand::None;
    }
    return commandFor(model_.selected());
}

MainMenuCommand MainMenuController::confirmQuit() noexcept {
    if (model_.modal() != MainMenuModal::QuitConfirm) {
        return MainMenuCommand::None;
    }
    model_.setModal(MainMenuModal::None);
    return MainMenuCommand::ExitApplication;
}

bool MainMenuController::cancel() noexcept {
    if (model_.modal() == MainMenuModal::None) {
        return false;
    }
    model_.setModal(MainMenuModal::None);
    return true;
}

bool MainMenuController::hover(MainMenuItem item) noexcept {
    if (model_.modal() != MainMenuModal::None) {
        return false;
    }
    return model_.select(item);
}

MainMenuCommand MainMenuController::commandFor(MainMenuItem item) noexcept {
    switch (item) {
    case MainMenuItem::Continue:
        return MainMenuCommand::ContinueLatest;
    case MainMenuItem::NewHotel:
        return MainMenuCommand::StartNewHotel;
    case MainMenuItem::LoadHotel:
        return MainMenuCommand::OpenLoadHotel;
    case MainMenuItem::Scenarios:
        return MainMenuCommand::OpenScenarios;
    case MainMenuItem::Sandbox:
        return MainMenuCommand::OpenSandbox;
    case MainMenuItem::Settings:
        return MainMenuCommand::OpenSettings;
    case MainMenuItem::Credits:
        return MainMenuCommand::OpenCredits;
    case MainMenuItem::Quit:
        return MainMenuCommand::ExitApplication;
    }
    return MainMenuCommand::None;
}

}  // namespace hh::frontend
