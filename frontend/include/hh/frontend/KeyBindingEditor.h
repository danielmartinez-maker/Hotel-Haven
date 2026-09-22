#pragma once

#include "hh/frontend/UiSettings.h"
#include <array>
#include <optional>
#include <string_view>

namespace hh::frontend {

enum class KeyBindingCaptureResult {
    Idle,
    Applied,
    Cancelled,
    Duplicate,
    Invalid
};

struct KeyBindingDescriptor {
    UiAction action;
    std::string_view label;
};

inline constexpr std::array<KeyBindingDescriptor, 7> EditableKeyBindings{{
    {UiAction::NavigatePrevious, "Previous"},
    {UiAction::NavigateNext, "Next"},
    {UiAction::Activate, "Activate"},
    {UiAction::Cancel, "Cancel"},
    {UiAction::SpeedDown, "Speed down"},
    {UiAction::SpeedUp, "Speed up"},
    {UiAction::PauseToggle, "Pause"},
}};

class KeyBindingEditor {
public:
    static constexpr int CancelCaptureKeyCode = 27;

    void begin(UiAction action) noexcept { pendingAction_ = action; }
    void cancel() noexcept { pendingAction_.reset(); }
    [[nodiscard]] bool capturing() const noexcept { return pendingAction_.has_value(); }
    [[nodiscard]] std::optional<UiAction> pendingAction() const noexcept { return pendingAction_; }

    KeyBindingCaptureResult capture(UiSettings& settings, int keyCode) noexcept {
        if (!pendingAction_)
            return KeyBindingCaptureResult::Idle;
        if (keyCode == CancelCaptureKeyCode) {
            pendingAction_.reset();
            return KeyBindingCaptureResult::Cancelled;
        }
        if (keyCode <= 0 || keyCode > 255)
            return KeyBindingCaptureResult::Invalid;
        if (const auto existing = settings.actionForKey(keyCode);
            existing && *existing != *pendingAction_)
            return KeyBindingCaptureResult::Duplicate;
        if (!settings.setKeyboardBinding(*pendingAction_, keyCode))
            return KeyBindingCaptureResult::Invalid;
        pendingAction_.reset();
        return KeyBindingCaptureResult::Applied;
    }

private:
    std::optional<UiAction> pendingAction_;
};

} // namespace hh::frontend
