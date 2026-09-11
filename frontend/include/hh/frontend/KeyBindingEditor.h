#pragma once

#include "hh/frontend/UiSettings.h"
#include <optional>

namespace hh::frontend {

enum class KeyBindingCaptureResult {
    Idle,
    Applied,
    Duplicate,
    Invalid
};

class KeyBindingEditor {
public:
    void begin(UiAction action) noexcept { pendingAction_ = action; }
    void cancel() noexcept { pendingAction_.reset(); }
    [[nodiscard]] bool capturing() const noexcept { return pendingAction_.has_value(); }
    [[nodiscard]] std::optional<UiAction> pendingAction() const noexcept { return pendingAction_; }

    KeyBindingCaptureResult capture(UiSettings& settings, int keyCode) noexcept {
        if (!pendingAction_)
            return KeyBindingCaptureResult::Idle;
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
