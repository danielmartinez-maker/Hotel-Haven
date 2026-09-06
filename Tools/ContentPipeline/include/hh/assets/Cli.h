#pragma once
#include <iosfwd>
#include <span>
#include <string_view>

namespace hh::assets {
int run_asset_cli(std::span<const std::string_view> args, std::ostream& out, std::ostream& err);
}
