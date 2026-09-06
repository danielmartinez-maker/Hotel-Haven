#include "hh/assets/Json.h"
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <stdexcept>
#include <string>

namespace hh::assets {
namespace {
[[noreturn]] void type_error(const char* expected) {
    throw std::runtime_error(std::string("JSON type mismatch, expected ") + expected);
}

void append_utf8(std::string& out, unsigned codepoint) {
    if (codepoint <= 0x7Fu) {
        out.push_back(static_cast<char>(codepoint));
    } else if (codepoint <= 0x7FFu) {
        out.push_back(static_cast<char>(0xC0u | (codepoint >> 6u)));
        out.push_back(static_cast<char>(0x80u | (codepoint & 0x3Fu)));
    } else if (codepoint <= 0xFFFFu) {
        out.push_back(static_cast<char>(0xE0u | (codepoint >> 12u)));
        out.push_back(static_cast<char>(0x80u | ((codepoint >> 6u) & 0x3Fu)));
        out.push_back(static_cast<char>(0x80u | (codepoint & 0x3Fu)));
    } else if (codepoint <= 0x10FFFFu) {
        out.push_back(static_cast<char>(0xF0u | (codepoint >> 18u)));
        out.push_back(static_cast<char>(0x80u | ((codepoint >> 12u) & 0x3Fu)));
        out.push_back(static_cast<char>(0x80u | ((codepoint >> 6u) & 0x3Fu)));
        out.push_back(static_cast<char>(0x80u | (codepoint & 0x3Fu)));
    } else {
        throw std::runtime_error("invalid Unicode codepoint");
    }
}

unsigned hex_digit(char c) {
    if (c >= '0' && c <= '9') return static_cast<unsigned>(c - '0');
    if (c >= 'a' && c <= 'f') return 10u + static_cast<unsigned>(c - 'a');
    if (c >= 'A' && c <= 'F') return 10u + static_cast<unsigned>(c - 'A');
    throw std::runtime_error("invalid JSON unicode escape");
}

class Parser {
public:
    explicit Parser(std::string_view text) : text_(text) {}

    JsonValue parse_document() {
        skip_ws();
        auto value = parse_value();
        skip_ws();
        if (pos_ != text_.size()) fail("trailing data");
        return value;
    }

private:
    [[noreturn]] void fail(const char* message) const {
        throw std::runtime_error(std::string("JSON parse error at byte ") + std::to_string(pos_) + ": " + message);
    }

    void skip_ws() {
        while (pos_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[pos_])) != 0) ++pos_;
    }

    bool consume(char expected) {
        if (pos_ < text_.size() && text_[pos_] == expected) { ++pos_; return true; }
        return false;
    }

    void expect(char expected) {
        if (!consume(expected)) fail("unexpected character");
    }

    JsonValue parse_value() {
        skip_ws();
        if (pos_ >= text_.size()) fail("unexpected end of input");
        switch (text_[pos_]) {
        case '{': return parse_object();
        case '[': return parse_array();
        case '"': return JsonValue(parse_string());
        case 't': consume_literal("true"); return JsonValue(true);
        case 'f': consume_literal("false"); return JsonValue(false);
        case 'n': consume_literal("null"); return JsonValue(nullptr);
        default:
            if (text_[pos_] == '-' || (text_[pos_] >= '0' && text_[pos_] <= '9')) return JsonValue(parse_number());
            fail("invalid value");
        }
    }

    JsonValue parse_object() {
        expect('{');
        JsonValue::Object object;
        skip_ws();
        if (consume('}')) return JsonValue(std::move(object));
        for (;;) {
            skip_ws();
            if (pos_ >= text_.size() || text_[pos_] != '"') fail("object key must be a string");
            auto key = parse_string();
            skip_ws();
            expect(':');
            auto [it, inserted] = object.emplace(std::move(key), parse_value());
            static_cast<void>(it);
            if (!inserted) fail("duplicate object key");
            skip_ws();
            if (consume('}')) break;
            expect(',');
        }
        return JsonValue(std::move(object));
    }

    JsonValue parse_array() {
        expect('[');
        JsonValue::Array array;
        skip_ws();
        if (consume(']')) return JsonValue(std::move(array));
        for (;;) {
            array.push_back(parse_value());
            skip_ws();
            if (consume(']')) break;
            expect(',');
        }
        return JsonValue(std::move(array));
    }

    unsigned parse_u16() {
        if (pos_ + 4 > text_.size()) fail("truncated unicode escape");
        unsigned value = 0;
        for (int i = 0; i < 4; ++i) value = (value << 4u) | hex_digit(text_[pos_++]);
        return value;
    }

    std::string parse_string() {
        expect('"');
        std::string out;
        while (pos_ < text_.size()) {
            const char c = text_[pos_++];
            if (c == '"') return out;
            if (static_cast<unsigned char>(c) < 0x20u) fail("unescaped control character");
            if (c != '\\') { out.push_back(c); continue; }
            if (pos_ >= text_.size()) fail("truncated escape");
            const char escape = text_[pos_++];
            switch (escape) {
            case '"': out.push_back('"'); break;
            case '\\': out.push_back('\\'); break;
            case '/': out.push_back('/'); break;
            case 'b': out.push_back('\b'); break;
            case 'f': out.push_back('\f'); break;
            case 'n': out.push_back('\n'); break;
            case 'r': out.push_back('\r'); break;
            case 't': out.push_back('\t'); break;
            case 'u': {
                unsigned cp = parse_u16();
                if (cp >= 0xD800u && cp <= 0xDBFFu) {
                    if (pos_ + 2 > text_.size() || text_[pos_] != '\\' || text_[pos_ + 1] != 'u') fail("missing low surrogate");
                    pos_ += 2;
                    const unsigned low = parse_u16();
                    if (low < 0xDC00u || low > 0xDFFFu) fail("invalid low surrogate");
                    cp = 0x10000u + ((cp - 0xD800u) << 10u) + (low - 0xDC00u);
                } else if (cp >= 0xDC00u && cp <= 0xDFFFu) {
                    fail("unexpected low surrogate");
                }
                append_utf8(out, cp);
                break;
            }
            default: fail("invalid escape");
            }
        }
        fail("unterminated string");
    }

    double parse_number() {
        const std::size_t start = pos_;
        if (consume('-') && pos_ >= text_.size()) fail("truncated number");
        if (consume('0')) {
            if (pos_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[pos_])) != 0) fail("leading zero");
        } else {
            if (pos_ >= text_.size() || text_[pos_] < '1' || text_[pos_] > '9') fail("invalid number");
            while (pos_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[pos_])) != 0) ++pos_;
        }
        if (consume('.')) {
            if (pos_ >= text_.size() || std::isdigit(static_cast<unsigned char>(text_[pos_])) == 0) fail("invalid fraction");
            while (pos_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[pos_])) != 0) ++pos_;
        }
        if (pos_ < text_.size() && (text_[pos_] == 'e' || text_[pos_] == 'E')) {
            ++pos_;
            if (pos_ < text_.size() && (text_[pos_] == '+' || text_[pos_] == '-')) ++pos_;
            if (pos_ >= text_.size() || std::isdigit(static_cast<unsigned char>(text_[pos_])) == 0) fail("invalid exponent");
            while (pos_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[pos_])) != 0) ++pos_;
        }
        const std::string token(text_.substr(start, pos_ - start));
        char* end = nullptr;
        const double value = std::strtod(token.c_str(), &end);
        if (end == nullptr || *end != '\0' || !std::isfinite(value)) fail("invalid numeric value");
        return value;
    }

    void consume_literal(std::string_view literal) {
        if (text_.substr(pos_, literal.size()) != literal) fail("invalid literal");
        pos_ += literal.size();
    }

    std::string_view text_;
    std::size_t pos_{};
};
}

bool JsonValue::as_bool() const { if (!is_bool()) type_error("boolean"); return std::get<bool>(value_); }
double JsonValue::as_number() const { if (!is_number()) type_error("number"); return std::get<double>(value_); }
const std::string& JsonValue::as_string() const { if (!is_string()) type_error("string"); return std::get<std::string>(value_); }
const JsonValue::Array& JsonValue::as_array() const { if (!is_array()) type_error("array"); return std::get<Array>(value_); }
const JsonValue::Object& JsonValue::as_object() const { if (!is_object()) type_error("object"); return std::get<Object>(value_); }
const JsonValue& JsonValue::at(std::string_view key) const {
    const auto& object = as_object();
    const auto it = object.find(key);
    if (it == object.end()) throw std::out_of_range("missing JSON key: " + std::string(key));
    return it->second;
}
const JsonValue* JsonValue::find(std::string_view key) const noexcept {
    if (!is_object()) return nullptr;
    const auto& object = std::get<Object>(value_);
    const auto it = object.find(key);
    return it == object.end() ? nullptr : &it->second;
}
JsonValue parse_json(std::string_view text) { return Parser(text).parse_document(); }
}
