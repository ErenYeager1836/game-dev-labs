#pragma once

#include "vec2.hpp"

namespace gamedev::physics2d {

struct Manifold {
    bool is_colliding = false;
    Vec2 normal{0.0f, 0.0f};      // Unit vector pointing from Body A towards Body B
    float penetration_depth = 0.0f; // Positive overlap distance
    Vec2 contact_point{0.0f, 0.0f};

    void reset() noexcept {
        is_colliding = false;
        normal = {0.0f, 0.0f};
        penetration_depth = 0.0f;
        contact_point = {0.0f, 0.0f};
    }
};

} // namespace gamedev::physics2d
