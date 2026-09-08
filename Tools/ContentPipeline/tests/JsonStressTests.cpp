#include "Test.h"
#include "hh/assets/Json.h"
#include <cstdint>
#include <stdexcept>
#include <string>

using namespace hh::assets;
namespace {
std::uint64_t next_random(std::uint64_t& state) noexcept {
    state ^= state << 13u;
    state ^= state >> 7u;
    state ^= state << 17u;
    return state;
}
}

HH_TEST("json parser rejects excessive nesting") {
    constexpr std::size_t depth = 257;
    std::string text(depth, '[');
    text += '0';
    text.append(depth, ']');
    bool threw = false;
    try { static_cast<void>(parse_json(text)); } catch (const std::runtime_error&) { threw = true; }
    HH_REQUIRE(threw);
}

HH_TEST("json parser rejects numeric overflow instead of producing infinity") {
    constexpr const char* cases[] = {"1e9999", "-1e9999"};
    for (const char* text : cases) {
        bool threw = false;
        try { static_cast<void>(parse_json(text)); } catch (const std::runtime_error&) { threw = true; }
        HH_REQUIRE(threw);
    }
}

HH_TEST("json parser decodes UTF-16 surrogate pairs in unicode escapes") {
    const auto value = parse_json("\"\\uD83D\\uDE00\"");
    HH_REQUIRE(value.as_string() == std::string("\xF0\x9F\x98\x80", 4));
}

HH_TEST("json parser rejects unpaired UTF-16 surrogates") {
    constexpr const char* cases[] = {
        "\"\\uD800\"",
        "\"\\uDC00\"",
        "\"\\uD800x\"",
        "\"\\uD800\\u0041\"",
    };
    for (const char* text : cases) {
        bool threw = false;
        try { static_cast<void>(parse_json(text)); } catch (const std::runtime_error&) { threw = true; }
        HH_REQUIRE(threw);
    }
}

HH_TEST("json parser survives five thousand deterministic hostile strings") {
    std::uint64_t state = 0x4a534f4e46555a5aull;
    constexpr char alphabet[] = "{}[],:\\\"0123456789truefalsenull abcdefABCDEF+-eE\\u\n\r\t";
    constexpr std::size_t alphabet_size = sizeof(alphabet) - 1u;
    for (int case_index = 0; case_index < 5000; ++case_index) {
        const std::size_t length = static_cast<std::size_t>(next_random(state) % 1025u);
        std::string text;
        text.reserve(length);
        for (std::size_t i = 0; i < length; ++i) {
            text.push_back(alphabet[next_random(state) % alphabet_size]);
        }
        try {
            static_cast<void>(parse_json(text));
        } catch (const std::runtime_error&) {
            // Malformed JSON is expected to fail closed with a parse/type error.
        }
    }
}
