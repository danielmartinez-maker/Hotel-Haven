#include "hh/assets/Hash.h"
#include <array>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace hh::assets {
namespace {
constexpr std::array<std::uint32_t, 64> kRound{
    0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,0x3956c25bu,0x59f111f1u,0x923f82a4u,0xab1c5ed5u,
    0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,
    0xe49b69c1u,0xefbe4786u,0x0fc19dc6u,0x240ca1ccu,0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
    0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,0xc6e00bf3u,0xd5a79147u,0x06ca6351u,0x14292967u,
    0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,
    0xa2bfe8a1u,0xa81a664bu,0xc24b8b70u,0xc76c51a3u,0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
    0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,0x391c0cb3u,0x4ed8aa4au,0x5b9cca4fu,0x682e6ff3u,
    0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u};
constexpr std::uint32_t rotr(std::uint32_t value, unsigned count) noexcept { return (value >> count) | (value << (32u - count)); }

class Sha256State {
public:
    void update(std::span<const std::byte> bytes) {
        for (const auto byte : bytes) {
            buffer_[buffer_size_++] = std::to_integer<std::uint8_t>(byte);
            ++byte_count_;
            if (buffer_size_ == buffer_.size()) {
                transform(buffer_.data());
                buffer_size_ = 0;
            }
        }
    }

    std::array<std::byte, 32> finish() {
        const std::uint64_t bit_count = byte_count_ * 8u;
        buffer_[buffer_size_++] = 0x80u;
        if (buffer_size_ > 56u) {
            while (buffer_size_ < 64u) buffer_[buffer_size_++] = 0u;
            transform(buffer_.data());
            buffer_size_ = 0;
        }
        while (buffer_size_ < 56u) buffer_[buffer_size_++] = 0u;
        for (int shift = 56; shift >= 0; shift -= 8) {
            buffer_[buffer_size_++] = static_cast<std::uint8_t>((bit_count >> static_cast<unsigned>(shift)) & 0xffu);
        }
        transform(buffer_.data());
        std::array<std::byte, 32> result{};
        for (std::size_t i = 0; i < state_.size(); ++i) {
            result[i * 4u] = static_cast<std::byte>((state_[i] >> 24u) & 0xffu);
            result[i * 4u + 1u] = static_cast<std::byte>((state_[i] >> 16u) & 0xffu);
            result[i * 4u + 2u] = static_cast<std::byte>((state_[i] >> 8u) & 0xffu);
            result[i * 4u + 3u] = static_cast<std::byte>(state_[i] & 0xffu);
        }
        return result;
    }

private:
    void transform(const std::uint8_t* block) {
        std::array<std::uint32_t, 64> w{};
        for (std::size_t i = 0; i < 16; ++i) {
            const std::size_t j = i * 4u;
            w[i] = (static_cast<std::uint32_t>(block[j]) << 24u) |
                   (static_cast<std::uint32_t>(block[j + 1u]) << 16u) |
                   (static_cast<std::uint32_t>(block[j + 2u]) << 8u) |
                   static_cast<std::uint32_t>(block[j + 3u]);
        }
        for (std::size_t i = 16; i < 64; ++i) {
            const std::uint32_t s0 = rotr(w[i - 15u], 7u) ^ rotr(w[i - 15u], 18u) ^ (w[i - 15u] >> 3u);
            const std::uint32_t s1 = rotr(w[i - 2u], 17u) ^ rotr(w[i - 2u], 19u) ^ (w[i - 2u] >> 10u);
            w[i] = w[i - 16u] + s0 + w[i - 7u] + s1;
        }
        std::uint32_t a = state_[0], b = state_[1], c = state_[2], d = state_[3];
        std::uint32_t e = state_[4], f = state_[5], g = state_[6], h = state_[7];
        for (std::size_t i = 0; i < 64; ++i) {
            const std::uint32_t s1 = rotr(e, 6u) ^ rotr(e, 11u) ^ rotr(e, 25u);
            const std::uint32_t choice = (e & f) ^ ((~e) & g);
            const std::uint32_t temp1 = h + s1 + choice + kRound[i] + w[i];
            const std::uint32_t s0 = rotr(a, 2u) ^ rotr(a, 13u) ^ rotr(a, 22u);
            const std::uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
            const std::uint32_t temp2 = s0 + majority;
            h = g; g = f; f = e; e = d + temp1; d = c; c = b; b = a; a = temp1 + temp2;
        }
        state_[0] += a; state_[1] += b; state_[2] += c; state_[3] += d;
        state_[4] += e; state_[5] += f; state_[6] += g; state_[7] += h;
    }

    std::array<std::uint32_t, 8> state_{0x6a09e667u,0xbb67ae85u,0x3c6ef372u,0xa54ff53au,0x510e527fu,0x9b05688cu,0x1f83d9abu,0x5be0cd19u};
    std::array<std::uint8_t, 64> buffer_{};
    std::size_t buffer_size_{};
    std::uint64_t byte_count_{};
};

std::string hex_digest(const std::array<std::byte, 32>& digest) {
    constexpr char hex[] = "0123456789abcdef";
    std::string out;
    out.reserve(64);
    for (const auto byte : digest) {
        const auto value = std::to_integer<unsigned>(byte);
        out.push_back(hex[(value >> 4u) & 0x0fu]);
        out.push_back(hex[value & 0x0fu]);
    }
    return out;
}
}

std::array<std::byte, 32> sha256(std::span<const std::byte> bytes) {
    Sha256State state;
    state.update(bytes);
    return state.finish();
}

std::string sha256_hex(std::span<const std::byte> bytes) { return hex_digest(sha256(bytes)); }

std::string hash_file(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("cannot hash file: " + path.string());
    Sha256State state;
    std::array<char, 64 * 1024> buffer{};
    while (input) {
        input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const auto count = input.gcount();
        if (count > 0) state.update(std::as_bytes(std::span(buffer.data(), static_cast<std::size_t>(count))));
    }
    if (!input.eof()) throw std::runtime_error("error reading file for hash: " + path.string());
    return hex_digest(state.finish());
}
}
