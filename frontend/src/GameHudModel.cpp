#include "hh/frontend/GameHudModel.h"
#include <cstdlib>
#include <iomanip>
#include <sstream>
namespace hh::frontend {
namespace {
std::string money(std::int64_t cents) {
    const bool negative = cents < 0;
    const std::uint64_t absolute = static_cast<std::uint64_t>(negative ? -(cents + 1) + 1 : cents);
    const std::uint64_t dollars = absolute / 100, remainder = absolute % 100;
    std::string digits = std::to_string(dollars);
    for (std::ptrdiff_t i = static_cast<std::ptrdiff_t>(digits.size()) - 3; i > 0; i -= 3) digits.insert(static_cast<std::size_t>(i), ",");
    std::ostringstream out; if (negative) out << '-'; out << '$' << digits << '.' << std::setw(2) << std::setfill('0') << remainder; return out.str();
}
std::string permille(int value) { std::ostringstream out; out << value / 10 << '.' << std::abs(value % 10) << '%'; return out.str(); }
}
void GameHudModel::update(const HudSnapshot& snapshot) { snapshot_ = snapshot; }
std::string GameHudModel::cashText() const { return money(snapshot_.cashCents); }
std::string GameHudModel::timeText() const { std::ostringstream out; out << "Day " << (snapshot_.day + 1) << ' ' << std::setw(2) << std::setfill('0') << snapshot_.hour << ':' << std::setw(2) << snapshot_.minute; return out.str(); }
std::string GameHudModel::occupancyText() const { return permille(snapshot_.occupancyPermille); }
std::string GameHudModel::satisfactionText() const { return permille(snapshot_.satisfactionPermille); }
std::string GameHudModel::reputationText() const { return permille(snapshot_.reputationPermille); }
} // namespace hh::frontend
