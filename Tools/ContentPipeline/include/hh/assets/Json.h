#pragma once
#include <map>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace hh::assets {
class JsonValue {
public:
    using Array = std::vector<JsonValue>;
    using Object = std::map<std::string, JsonValue, std::less<>>;
    using Storage = std::variant<std::nullptr_t, bool, double, std::string, Array, Object>;

    JsonValue() : value_(nullptr) {}
    explicit JsonValue(std::nullptr_t) : value_(nullptr) {}
    explicit JsonValue(bool value) : value_(value) {}
    explicit JsonValue(double value) : value_(value) {}
    explicit JsonValue(std::string value) : value_(std::move(value)) {}
    explicit JsonValue(Array value) : value_(std::move(value)) {}
    explicit JsonValue(Object value) : value_(std::move(value)) {}

    bool is_null() const noexcept { return std::holds_alternative<std::nullptr_t>(value_); }
    bool is_bool() const noexcept { return std::holds_alternative<bool>(value_); }
    bool is_number() const noexcept { return std::holds_alternative<double>(value_); }
    bool is_string() const noexcept { return std::holds_alternative<std::string>(value_); }
    bool is_array() const noexcept { return std::holds_alternative<Array>(value_); }
    bool is_object() const noexcept { return std::holds_alternative<Object>(value_); }

    bool as_bool() const;
    double as_number() const;
    const std::string& as_string() const;
    const Array& as_array() const;
    const Object& as_object() const;
    const JsonValue& at(std::string_view key) const;
    const JsonValue* find(std::string_view key) const noexcept;

private:
    Storage value_;
};

JsonValue parse_json(std::string_view text);
}
