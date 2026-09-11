#pragma once
#include "hh/frontend/GameUiTypes.h"
#include <optional>
#include <span>
#include <string>
#include <vector>
namespace hh::frontend {
struct OperationsFilter {
    std::optional<OperationArea> area;
    std::string taskType;
    std::optional<int> minimumPriority;
    std::string state;
    std::string reasonCode;
    std::optional<EntityId> assigneeId;
    std::optional<int> floor;
    std::optional<int> x;
    std::optional<int> y;
};
class OperationsDashboard {
public:
    void update(const OperationsSnapshot& snapshot);
    [[nodiscard]] const OperationsSnapshot& snapshot() const noexcept { return snapshot_; }
    [[nodiscard]] std::vector<OperationRow> filtered(OperationArea area) const;
    [[nodiscard]] std::size_t filteredCount(OperationArea area) const noexcept;
    [[nodiscard]] std::size_t filteredCount(const OperationsFilter& filter) const noexcept;
    [[nodiscard]] std::span<const OperationRow> window(std::size_t offset, std::size_t count) const noexcept;
    [[nodiscard]] std::vector<OperationRow> filteredWindow(OperationArea area, std::size_t offset, std::size_t count) const;
    [[nodiscard]] std::vector<OperationRow> filteredWindow(const OperationsFilter& filter, std::size_t offset, std::size_t count) const;
private: OperationsSnapshot snapshot_{};
};
} // namespace hh::frontend
