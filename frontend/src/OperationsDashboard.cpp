#include "hh/frontend/OperationsDashboard.h"
#include <algorithm>
#include <iterator>
namespace hh::frontend {
void OperationsDashboard::update(const OperationsSnapshot& snapshot) { snapshot_ = snapshot; }
std::vector<OperationRow> OperationsDashboard::filtered(OperationArea area) const { std::vector<OperationRow> result; std::copy_if(snapshot_.rows.begin(), snapshot_.rows.end(), std::back_inserter(result), [area](const OperationRow& row) { return row.area == area; }); return result; }
std::size_t OperationsDashboard::filteredCount(OperationArea area) const noexcept { return static_cast<std::size_t>(std::count_if(snapshot_.rows.begin(), snapshot_.rows.end(), [area](const OperationRow& row) { return row.area == area; })); }
std::span<const OperationRow> OperationsDashboard::window(std::size_t offset, std::size_t count) const noexcept { if (offset >= snapshot_.rows.size()) return {}; const auto available = snapshot_.rows.size() - offset, size = std::min(count, available); return {snapshot_.rows.data() + offset, size}; }
std::vector<OperationRow> OperationsDashboard::filteredWindow(OperationArea area, std::size_t offset, std::size_t count) const {
    if (count == 0)
        return {};
    std::vector<OperationRow> result;
    result.reserve(std::min(count, snapshot_.rows.size()));
    std::size_t matched = 0;
    for (const auto& row : snapshot_.rows) {
        if (row.area != area)
            continue;
        if (matched++ < offset)
            continue;
        result.push_back(row);
        if (result.size() == count)
            break;
    }
    return result;
}
} // namespace hh::frontend
