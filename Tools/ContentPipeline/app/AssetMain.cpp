#include "hh/assets/Cli.h"
#include <iostream>
#include <string_view>
#include <vector>
int main(int argc, char** argv) {
    std::vector<std::string_view> args;
    args.reserve(argc > 1 ? static_cast<std::size_t>(argc - 1) : 0u);
    for (int i = 1; i < argc; ++i) args.emplace_back(argv[i]);
    return hh::assets::run_asset_cli(args, std::cout, std::cerr);
}
