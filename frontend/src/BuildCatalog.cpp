#include "hh/frontend/BuildCatalog.h"
#include <algorithm>
#include <cctype>
namespace hh::frontend {
namespace { std::string lower(std::string value) { std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); }); return value; } }
std::vector<BuildCatalogItem> BuildCatalog::filter(const std::string& query, const std::string& category) const { const auto q = lower(query), c = lower(category); std::vector<BuildCatalogItem> result; for (const auto& item : items_) { if (!c.empty() && lower(item.category) != c) continue; if (!q.empty() && lower(item.name).find(q) == std::string::npos && lower(item.id).find(q) == std::string::npos) continue; result.push_back(item); } return result; }
} // namespace hh::frontend
