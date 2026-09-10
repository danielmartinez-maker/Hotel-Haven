#include "hh/frontend/OperationsDashboard.h"
#include <algorithm>
#include <iterator>
namespace hh::frontend {
void OperationsDashboard::update(const OperationsSnapshot& snapshot) { snapshot_ = snapshot; }
std::vector<OperationRow> OperationsDashboard::filtered(OperationArea area) const { std::vector<OperationRow> result; std::copy_if(snapshot_.rows.begin(), snapshot_.rows.end(), std::back_inserter(result), [area](const OperationRow& row) { return row.area == area; }); return result; }
std::span<const OperationRow> OperationsDashboard::window(std::size_t offset, std::size_t count) const noexcept { if (offset >= snapshot_.rows.size()) return {}; const auto available = snapshot_.rows.size() - offset, size = std::min(count, available); return {snapshot_.rows.data() + offset, size}; }
} // namespace hh::frontend
