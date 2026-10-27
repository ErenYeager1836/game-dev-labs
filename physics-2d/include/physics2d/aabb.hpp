#pragma once

#include "vec2.hpp"
#include <algorithm>

namespace gamedev::physics2d {

struct AABB {
    Vec2 min{0.0f, 0.0f};
    Vec2 max{0.0f, 0.0f};

    constexpr AABB() = default;
    constexpr AABB(Vec2 min_, Vec2 max_) : min(min_), max(max_) {}

    constexpr bool contains(Vec2 point) const noexcept {
        return point.x >= min.x && point.x <= max.x &&
               point.y >= min.y && point.y <= max.y;
    }

    constexpr bool intersects(const AABB& o) const noexcept {
        if (max.x < o.min.x || min.x > o.max.x) return false;
        if (max.y < o.min.y || min.y > o.max.y) return false;
        return true;
    }

    constexpr Vec2 extents() const noexcept {
        return (max - min) * 0.5f;
    }

    constexpr Vec2 center() const noexcept {
        return (min + max) * 0.5f;
    }
};

} // namespace gamedev::physics2d
