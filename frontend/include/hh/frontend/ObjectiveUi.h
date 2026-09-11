#pragma once
#include "hh/frontend/GameUiTypes.h"
#include <cstdint>
#include <unordered_set>
#include <vector>
namespace hh::frontend {
class ObjectiveUi {
public:
    void update(const ObjectivesSnapshot& snapshot); [[nodiscard]] const std::vector<ObjectiveSnapshot>& items() const noexcept { return visibleItems_; }
    [[nodiscard]] bool dismiss(std::uint64_t objectiveId); [[nodiscard]] bool minimized() const noexcept { return minimized_; }
    void setMinimized(bool minimized) noexcept { minimized_ = minimized; }
private: ObjectivesSnapshot snapshot_{}; std::vector<ObjectiveSnapshot> visibleItems_; std::unordered_set<std::uint64_t> dismissed_; bool minimized_{}; void rebuildVisible();
};
} // namespace hh::frontend
