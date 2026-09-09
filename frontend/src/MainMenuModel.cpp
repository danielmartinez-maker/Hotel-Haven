#include "hh/frontend/MainMenuModel.h"

#include <algorithm>

namespace hh::frontend {
namespace {

constexpr std::array<MainMenuItem, 8> kMenuOrder{
    MainMenuItem::Continue,
    MainMenuItem::NewHotel,
    MainMenuItem::LoadHotel,
    MainMenuItem::Scenarios,
    MainMenuItem::Sandbox,
    MainMenuItem::Settings,
    MainMenuItem::Credits,
    MainMenuItem::Quit,
};

}  // namespace

MainMenuModel::MainMenuModel(bool hasValidSave) noexcept {
    enabled_.fill(true);
    enabled_[indexOf(MainMenuItem::Continue)] = hasValidSave;
    selected_ = hasValidSave ? MainMenuItem::Continue : MainMenuItem::NewHotel;
}

const std::array<MainMenuItem, 8>& MainMenuModel::orderedItems() noexcept {
    return kMenuOrder;
}

std::size_t MainMenuModel::indexOf(MainMenuItem item) noexcept {
    const auto found = std::find(kMenuOrder.begin(), kMenuOrder.end(), item);
    return found == kMenuOrder.end()
               ? std::size_t{0}
               : static_cast<std::size_t>(std::distance(kMenuOrder.begin(), found));
}

bool MainMenuModel::isEnabled(MainMenuItem item) const noexcept {
    return enabled_[indexOf(item)];
}

bool MainMenuModel::select(MainMenuItem item) noexcept {
    if (!isEnabled(item)) {
        return false;
    }
    selected_ = item;
    return true;
}

void MainMenuModel::setEnabled(MainMenuItem item, bool enabled) noexcept {
    const std::size_t index = indexOf(item);
    enabled_[index] = enabled;
    if (!enabled && selected_ == item) {
        selected_ = firstEnabled();
    }
}

void MainMenuModel::setContinueAvailable(bool available) noexcept {
    setEnabled(MainMenuItem::Continue, available);
    if (available && selected_ == MainMenuItem::NewHotel) {
        selected_ = MainMenuItem::Continue;
    }
}

MainMenuItem MainMenuModel::firstEnabled() const noexcept {
    for (const MainMenuItem item : kMenuOrder) {
        if (isEnabled(item)) {
            return item;
        }
    }
    return MainMenuItem::NewHotel;
}

}  // namespace hh::frontend
