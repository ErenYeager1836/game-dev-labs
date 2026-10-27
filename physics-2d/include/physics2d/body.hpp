#pragma once

#include "aabb.hpp"
#include "shapes.hpp"
#include "vec2.hpp"

#include <memory>

namespace gamedev::physics2d {

class RigidBody {
public:
    Vec2 position{0.0f, 0.0f};
    Vec2 velocity{0.0f, 0.0f};
    Vec2 force{0.0f, 0.0f};

    float rotation = 0.0f; // In radians
    float angular_velocity = 0.0f;

    float mass = 1.0f;
    float inv_mass = 1.0f;

    float restitution = 0.5f; // Bounciness coefficient [0.0, 1.0]
    float friction = 0.2f;    // Static/dynamic friction coefficient

    std::shared_ptr<Shape> shape;

    RigidBody() = default;

    RigidBody(std::shared_ptr<Shape> shape_, float mass_ = 1.0f, Vec2 pos = {0.0f, 0.0f})
        : position(pos), shape(std::move(shape_)) {
        set_mass(mass_);
    }

    void set_mass(float m) noexcept {
        mass = m;
        if (m <= 0.0f) {
            mass = 0.0f;
            inv_mass = 0.0f; // Static/infinite mass body
        } else {
            inv_mass = 1.0f / m;
        }
    }

    bool is_static() const noexcept {
        return inv_mass == 0.0f;
    }

    void apply_force(Vec2 f) noexcept {
        if (!is_static()) {
            force += f;
        }
    }

    void apply_impulse(Vec2 impulse) noexcept {
        if (!is_static()) {
            velocity += impulse * inv_mass;
        }
    }

    AABB compute_aabb() const {
        if (shape) {
            return shape->compute_aabb(position, rotation);
        }
        return AABB(position, position);
    }
};

} // namespace gamedev::physics2d
