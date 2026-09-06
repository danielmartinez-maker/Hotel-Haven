#pragma once
#include <array>
#include <cstddef>
#include <filesystem>
#include <span>
#include <string>

namespace hh::assets {
std::array<std::byte, 32> sha256(std::span<const std::byte> bytes);
std::string sha256_hex(std::span<const std::byte> bytes);
std::string hash_file(const std::filesystem::path& path);
}
