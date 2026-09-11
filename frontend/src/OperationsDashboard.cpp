#include "hh/frontend/OperationsDashboard.h"
#include <algorithm>
#include <iterator>
namespace hh::frontend {
namespace {
bool matches(const OperationRow& row, const OperationsFilter& filter) noexcept {
    if (filter.area && row.area != *filter.area) return false;
    if (!filter.taskType.empty() && row.name != filter.taskType) return false;
    if (filter.minimumPriority && row.priority < *filter.minimumPriority) return false;
    if (!filter.state.empty() && row.state != filter.state) return false;
    if (!filter.reasonCode.empty() && row.reasonCode != filter.reasonCode) return false;
    if (filter.assigneeId && row.assigneeId != *filter.assigneeId) return false;
    if (filter.floor && row.targetFloor != *filter.floor) return false;
    if (filter.x && row.targetX != *filter.x) return false;
    if (filter.y && row.targetY != *filter.y) return false;
    return true;
}
OperationsFilter areaFilter(OperationArea area) {
    OperationsFilter filter;
    filter.area = area;
    return filter;
}
}
void OperationsDashboard::update(const OperationsSnapshot& snapshot) { snapshot_ = snapshot; }
std::vector<OperationRow> OperationsDashboard::filtered(OperationArea area) const { std::vector<OperationRow> result; std::copy_if(snapshot_.rows.begin(), snapshot_.rows.end(), std::back_inserter(result), [area](const OperationRow& row) { return row.area == area; }); return result; }
std::size_t OperationsDashboard::filteredCount(OperationArea area) const noexcept { return filteredCount(areaFilter(area)); }
std::size_t OperationsDashboard::filteredCount(const OperationsFilter& filter) const noexcept { return static_cast<std::size_t>(std::count_if(snapshot_.rows.begin(), snapshot_.rows.end(), [&filter](const OperationRow& row) { return matches(row, filter); })); }
std::span<const OperationRow> OperationsDashboard::window(std::size_t offset, std::size_t count) const noexcept { if (offset >= snapshot_.rows.size()) return {}; const auto available = snapshot_.rows.size() - offset, size = std::min(count, available); return {snapshot_.rows.data() + offset, size}; }
std::vector<OperationRow> OperationsDashboard::filteredWindow(OperationArea area, std::size_t offset, std::size_t count) const { return filteredWindow(areaFilter(area), offset, count); }
std::vector<OperationRow> OperationsDashboard::filteredWindow(const OperationsFilter& filter, std::size_t offset, std::size_t count) const {
    if (count == 0) return {};
    std::vector<OperationRow> result;
    result.reserve(std::min(count, snapshot_.rows.size()));
    std::size_t matched = 0;
    for (const auto& row : snapshot_.rows) {
        if (!matches(row, filter)) continue;
        if (matched++ < offset) continue;
        result.push_back(row);
        if (result.size() == count) break;
    }
    return result;
}
} // namespace hh::frontend
