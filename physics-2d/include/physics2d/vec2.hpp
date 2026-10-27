#pragma once

#include <cmath>
#include <iostream>

namespace gamedev::physics2d {

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    constexpr Vec2() = default;
    constexpr Vec2(float x_, float y_) : x(x_), y(y_) {}

    constexpr Vec2 operator+(const Vec2& o) const noexcept { return {x + o.x, y + o.y}; }
    constexpr Vec2 operator-(const Vec2& o) const noexcept { return {x - o.x, y - o.y}; }
    constexpr Vec2 operator*(float scalar) const noexcept { return {x * scalar, y * scalar}; }
    constexpr Vec2 operator/(float scalar) const noexcept { return {x / scalar, y / scalar}; }
    constexpr Vec2 operator-() const noexcept { return {-x, -y}; }

    Vec2& operator+=(const Vec2& o) noexcept { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(const Vec2& o) noexcept { x -= o.x; y -= o.y; return *this; }
    Vec2& operator*=(float scalar) noexcept { x *= scalar; y *= scalar; return *this; }
    Vec2& operator/=(float scalar) noexcept { x /= scalar; y /= scalar; return *this; }

    constexpr float dot(const Vec2& o) const noexcept { return x * o.x + y * o.y; }
    constexpr float cross(const Vec2& o) const noexcept { return x * o.y - y * o.x; }

    constexpr float length_squared() const noexcept { return x * x + y * y; }
    float length() const noexcept { return std::sqrt(length_squared()); }

    Vec2 normalized() const noexcept {
        float len = length();
        if (len > 1e-6f) {
            return *this / len;
        }
        return {0.0f, 0.0f};
    }

    constexpr Vec2 perp() const noexcept { return {-y, x}; }

    Vec2 rotated(float radians) const noexcept {
        float c = std::cos(radians);
        float s = std::sin(radians);
        return {x * c - y * s, x * s + y * c};
    }

    Vec2 reflect(const Vec2& normal) const noexcept {
        return *this - normal * (2.0f * dot(normal));
    }
};

inline constexpr Vec2 operator*(float scalar, const Vec2& v) noexcept {
    return v * scalar;
}

inline std::ostream& operator<<(std::ostream& os, const Vec2& v) {
    return os << "(" << v.x << ", " << v.y << ")";
}

} // namespace gamedev::physics2d
