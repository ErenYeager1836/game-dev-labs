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
    float torque = 0.0f;

    float mass = 1.0f;
    float inv_mass = 1.0f;
    float inertia = 1.0f;
    float inv_inertia = 1.0f;

    float restitution = 0.5f; // Bounciness coefficient [0.0, 1.0]
    float friction = 0.2f;    // Static/dynamic friction coefficient

    std::shared_ptr<Shape> shape;

    RigidBody() = default;

    RigidBody(std::shared_ptr<Shape> shape_, float mass_ = 1.0f, Vec2 pos = {0.0f, 0.0f})
        : position(pos), shape(std::move(shape_)) {
        set_mass(mass_);
    }

    void set_inertia(float i) noexcept {
        inertia = i;
        if (i <= 0.0f) {
            inertia = 0.0f;
            inv_inertia = 0.0f;
        } else {
            inv_inertia = 1.0f / i;
        }
    }

    void set_mass(float m) noexcept {
        mass = m;
        if (m <= 0.0f) {
            mass = 0.0f;
            inv_mass = 0.0f; // Static/infinite mass body
            inertia = 0.0f;
            inv_inertia = 0.0f;
        } else {
            inv_mass = 1.0f / m;
            compute_default_inertia();
        }
    }

    void compute_default_inertia() noexcept {
        if (mass <= 0.0f || !shape) {
            set_inertia(0.0f);
            return;
        }

        if (shape->type() == ShapeType::Circle) {
            const auto& circle = static_cast<const CircleShape&>(*shape);
            // Solid disk: I = 0.5 * m * r^2
            set_inertia(0.5f * mass * circle.radius * circle.radius);
        } else {
            // Box / Polygon: approximated via AABB dimensions I = 1/12 * m * (w^2 + h^2)
            AABB local_box = shape->compute_aabb({0.0f, 0.0f}, 0.0f);
            float w = local_box.max.x - local_box.min.x;
            float h = local_box.max.y - local_box.min.y;
            set_inertia((1.0f / 12.0f) * mass * (w * w + h * h));
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

    void apply_torque(float t) noexcept {
        if (!is_static()) {
            torque += t;
        }
    }

    void apply_force_at_point(Vec2 f, Vec2 contact_pt) noexcept {
        if (!is_static()) {
            apply_force(f);
            Vec2 r = contact_pt - position;
            apply_torque(r.cross(f));
        }
    }

    void apply_impulse(Vec2 impulse) noexcept {
        if (!is_static()) {
            velocity += impulse * inv_mass;
        }
    }

    void apply_impulse_at_point(Vec2 impulse, Vec2 contact_pt) noexcept {
        if (!is_static()) {
            velocity += impulse * inv_mass;
            Vec2 r = contact_pt - position;
            angular_velocity += r.cross(impulse) * inv_inertia;
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
