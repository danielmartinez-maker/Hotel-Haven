#pragma once
#include "hh/frontend/GameUiTypes.h"
#include <cstddef>
#include <optional>
#include <unordered_map>
#include <vector>
namespace hh::frontend {
class AlertCenter {
public:
    explicit AlertCenter(std::size_t resolvedHistoryLimit = 200) : resolvedHistoryLimit_(resolvedHistoryLimit) {}
    void ingest(const std::vector<AlertSnapshot>& alerts);
    [[nodiscard]] const std::vector<AlertSnapshot>& active() const noexcept { return active_; }
    [[nodiscard]] const std::vector<AlertSnapshot>& resolvedHistory() const noexcept { return resolved_; }
    [[nodiscard]] std::vector<AlertSnapshot> causalChain(std::uint64_t alertId) const;
    [[nodiscard]] std::optional<UiCommand> navigationFor(std::uint64_t alertId) const;
private:
    std::size_t resolvedHistoryLimit_; std::vector<AlertSnapshot> active_; std::vector<AlertSnapshot> resolved_; std::unordered_map<std::uint64_t, AlertSnapshot> known_;
};
} // namespace hh::frontend
