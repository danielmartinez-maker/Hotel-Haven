#include "Test.h"
#include "hh/assets/Json.h"
#include <cstdint>
#include <initializer_list>
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

std::string quoted_bytes(std::initializer_list<unsigned char> bytes) {
    std::string text;
    text.reserve(bytes.size() + 2u);
    text.push_back('"');
    for (const unsigned char byte : bytes) text.push_back(static_cast<char>(byte));
    text.push_back('"');
    return text;
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

HH_TEST("json parser rejects numeric underflow instead of silently producing zero") {
    constexpr const char* cases[] = {"1e-9999", "-1e-9999"};
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

HH_TEST("json parser accepts valid raw UTF-8") {
    const auto text = quoted_bytes({0x63u, 0x61u, 0x66u, 0xC3u, 0xA9u, 0x20u, 0xF0u, 0x9Fu, 0x98u, 0x80u});
    const auto value = parse_json(text);
    HH_REQUIRE(value.as_string() == text.substr(1u, text.size() - 2u));
}

HH_TEST("json parser rejects malformed raw UTF-8") {
    const std::string cases[] = {
        quoted_bytes({0x80u}),                         // isolated continuation
        quoted_bytes({0xC0u, 0xAFu}),                 // overlong two-byte encoding
        quoted_bytes({0xC2u}),                        // truncated two-byte sequence
        quoted_bytes({0xE2u, 0x28u, 0xA1u}),          // invalid continuation byte
        quoted_bytes({0xEDu, 0xA0u, 0x80u}),          // UTF-8 encoded surrogate
        quoted_bytes({0xF4u, 0x90u, 0x80u, 0x80u}),   // code point above U+10FFFF
        quoted_bytes({0xF8u, 0x88u, 0x80u, 0x80u, 0x80u}), // obsolete five-byte form
    };
    for (const auto& text : cases) {
        bool threw = false;
        try { static_cast<void>(parse_json(text)); } catch (const std::runtime_error&) { threw = true; }
        HH_REQUIRE(threw);
    }
}

HH_TEST("json parser accepts only RFC JSON whitespace outside strings") {
    constexpr const char* valid_cases[] = {
        " \t\r\n0 \t\r\n",
        "\t[\r\n true, false ]\t",
    };
    for (const char* text : valid_cases) {
        static_cast<void>(parse_json(text));
    }

    const std::string invalid_cases[] = {
        std::string("\v0"),
        std::string("\f0"),
        std::string("0\v"),
        std::string("0\f"),
        std::string("[0,\v1]"),
        std::string("{\"a\":\f1}"),
    };
    for (const auto& text : invalid_cases) {
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
