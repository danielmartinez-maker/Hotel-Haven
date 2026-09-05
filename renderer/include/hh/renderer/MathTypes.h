#pragma once

namespace hh::renderer {

struct Vec3 {
    float x{};
    float y{};
    float z{};
};

constexpr Vec3 operator+(Vec3 lhs, Vec3 rhs) noexcept {
    return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
}

constexpr Vec3 operator-(Vec3 lhs, Vec3 rhs) noexcept {
    return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}

constexpr Vec3 operator*(Vec3 value, float scalar) noexcept {
    return {value.x * scalar, value.y * scalar, value.z * scalar};
}

struct Color {
    float r{1.0f};
    float g{1.0f};
    float b{1.0f};
    float a{1.0f};
};

struct Aabb {
    Vec3 min{};
    Vec3 max{};
};

}  // namespace hh::renderer
