#pragma once

#include "body.hpp"
#include "collision.hpp"
#include "manifold.hpp"
#include "vec2.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

namespace gamedev::physics2d {

class PhysicsWorld {
public:
    Vec2 gravity{0.0f, -9.81f};

    PhysicsWorld() = default;
    explicit PhysicsWorld(Vec2 grav) : gravity(grav) {}

    RigidBody* add_body(std::unique_ptr<RigidBody> body) {
        bodies_.push_back(std::move(body));
        return bodies_.back().get();
    }

    RigidBody* create_body(std::shared_ptr<Shape> shape, float mass = 1.0f, Vec2 pos = {0.0f, 0.0f}) {
        return add_body(std::make_unique<RigidBody>(shape, mass, pos));
    }

    const std::vector<std::unique_ptr<RigidBody>>& bodies() const noexcept {
        return bodies_;
    }

    void clear() {
        bodies_.clear();
    }

    void step(float dt) {
        if (dt <= 0.0f) return;

        // 1. Integrate forces and gravity into linear velocity
        for (auto& body : bodies_) {
            if (!body->is_static()) {
                body->velocity += (gravity + body->force * body->inv_mass) * dt;
                body->force = {0.0f, 0.0f};
            }
        }

        // 2. Broadphase & Narrowphase Collision Detection + Impulse Resolution
        size_t count = bodies_.size();
        for (size_t i = 0; i < count; ++i) {
            for (size_t j = i + 1; j < count; ++j) {
                RigidBody& a = *bodies_[i];
                RigidBody& b = *bodies_[j];

                // Skip collision check between two static bodies
                if (a.is_static() && b.is_static()) {
                    continue;
                }

                // Broadphase: AABB pruning
                AABB aabbA = a.compute_aabb();
                AABB aabbB = b.compute_aabb();
                if (!aabbA.intersects(aabbB)) {
                    continue;
                }

                // Narrowphase: SAT / Circle Collision
                Manifold manifold;
                if (detect_collision(a, b, manifold)) {
                    resolve_collision(a, b, manifold);
                }
            }
        }

        // 3. Integrate velocities into positions
        for (auto& body : bodies_) {
            if (!body->is_static()) {
                body->position += body->velocity * dt;
                body->rotation += body->angular_velocity * dt;
            }
        }
    }

private:
    bool detect_collision(const RigidBody& a, const RigidBody& b, Manifold& manifold) {
        if (!a.shape || !b.shape) return false;

        ShapeType typeA = a.shape->type();
        ShapeType typeB = b.shape->type();

        if (typeA == ShapeType::Circle && typeB == ShapeType::Circle) {
            return CollisionDetector::test_circle_circle(
                static_cast<const CircleShape&>(*a.shape), a.position,
                static_cast<const CircleShape&>(*b.shape), b.position,
                manifold);
        }

        if (typeA == ShapeType::Polygon && typeB == ShapeType::Polygon) {
            return CollisionDetector::test_polygon_polygon(
                static_cast<const PolygonShape&>(*a.shape), a.position, a.rotation,
                static_cast<const PolygonShape&>(*b.shape), b.position, b.rotation,
                manifold);
        }

        if (typeA == ShapeType::Circle && typeB == ShapeType::Polygon) {
            return CollisionDetector::test_circle_polygon(
                static_cast<const CircleShape&>(*a.shape), a.position,
                static_cast<const PolygonShape&>(*b.shape), b.position, b.rotation,
                manifold);
        }

        if (typeA == ShapeType::Polygon && typeB == ShapeType::Circle) {
            bool colliding = CollisionDetector::test_circle_polygon(
                static_cast<const CircleShape&>(*b.shape), b.position,
                static_cast<const PolygonShape&>(*a.shape), a.position, a.rotation,
                manifold);
            if (colliding) {
                // Invert normal so it points from A to B
                manifold.normal = -manifold.normal;
            }
            return colliding;
        }

        return false;
    }

    void resolve_collision(RigidBody& a, RigidBody& b, const Manifold& m) {
        float total_inv_mass = a.inv_mass + b.inv_mass;
        if (total_inv_mass <= 0.0f) {
            return;
        }

        // 1. Positional Correction (Projection) to resolve overlap and prevent sinking
        constexpr float slop = 0.01f;     // Penetration allowance
        constexpr float percent = 0.4f;  // Slop correction percentage
        float correction_mag = std::max(m.penetration_depth - slop, 0.0f) / total_inv_mass * percent;
        Vec2 correction = m.normal * correction_mag;

        if (!a.is_static()) a.position -= correction * a.inv_mass;
        if (!b.is_static()) b.position += correction * b.inv_mass;

        // 2. Normal Velocity & Elastic Impulse
        Vec2 rv = b.velocity - a.velocity;
        float vel_along_normal = rv.dot(m.normal);

        // Do not resolve if velocities are already separating
        if (vel_along_normal > 0.0f) {
            return;
        }

        float e = std::min(a.restitution, b.restitution);
        float j = -(1.0f + e) * vel_along_normal / total_inv_mass;
        Vec2 normal_impulse = m.normal * j;

        if (!a.is_static()) a.velocity -= normal_impulse * a.inv_mass;
        if (!b.is_static()) b.velocity += normal_impulse * b.inv_mass;

        // 3. Tangent Velocity & Friction Impulse
        rv = b.velocity - a.velocity;
        vel_along_normal = rv.dot(m.normal);
        Vec2 tangent = rv - m.normal * vel_along_normal;
        float tangent_len = tangent.length();

        if (tangent_len > 1e-5f) {
            tangent /= tangent_len;
            float jt = -rv.dot(tangent) / total_inv_mass;

            // Coulomb's Law: clamp friction impulse by normal impulse * friction coefficient
            float mu = std::sqrt(a.friction * b.friction);
            float max_friction = std::abs(j) * mu;
            jt = std::clamp(jt, -max_friction, max_friction);

            Vec2 friction_impulse = tangent * jt;
            if (!a.is_static()) a.velocity -= friction_impulse * a.inv_mass;
            if (!b.is_static()) b.velocity += friction_impulse * b.inv_mass;
        }
    }

    std::vector<std::unique_ptr<RigidBody>> bodies_;
};

} // namespace gamedev::physics2d
