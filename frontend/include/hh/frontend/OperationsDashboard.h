#pragma once
#include "hh/frontend/GameUiTypes.h"
#include <span>
#include <vector>
namespace hh::frontend {
class OperationsDashboard {
public:
    void update(const OperationsSnapshot& snapshot);
    [[nodiscard]] const OperationsSnapshot& snapshot() const noexcept { return snapshot_; }
    [[nodiscard]] std::vector<OperationRow> filtered(OperationArea area) const;
    [[nodiscard]] std::span<const OperationRow> window(std::size_t offset, std::size_t count) const noexcept;
    [[nodiscard]] std::vector<OperationRow> filteredWindow(OperationArea area, std::size_t offset, std::size_t count) const;
private: OperationsSnapshot snapshot_{};
};
} // namespace hh::frontend
