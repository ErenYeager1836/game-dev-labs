#include "physics2d/aabb.hpp"
#include "physics2d/body.hpp"
#include "physics2d/collision.hpp"
#include "physics2d/shapes.hpp"
#include "physics2d/vec2.hpp"
#include "physics2d/world.hpp"

#include <cassert>
#include <cmath>
#include <iostream>

using namespace gamedev::physics2d;

void test_vec2_operations() {
    std::cout << "[+] Running test_vec2_operations...\n";
    Vec2 a(3.0f, 4.0f);
    assert(std::abs(a.length() - 5.0f) < 1e-5f);

    Vec2 norm = a.normalized();
    assert(std::abs(norm.length() - 1.0f) < 1e-5f);

    Vec2 b(1.0f, 0.0f);
    assert(a.dot(b) == 3.0f);
    assert(a.cross(b) == -4.0f);

    // Reflection test: incoming vector (1, -1) bouncing off floor with normal (0, 1)
    Vec2 incoming(1.0f, -1.0f);
    Vec2 normal(0.0f, 1.0f);
    Vec2 reflected = incoming.reflect(normal);
    assert(std::abs(reflected.x - 1.0f) < 1e-5f);
    assert(std::abs(reflected.y - 1.0f) < 1e-5f);

    std::cout << "    PASSED\n";
}

void test_aabb_intersections() {
    std::cout << "[+] Running test_aabb_intersections...\n";
    AABB box1(Vec2(0.0f, 0.0f), Vec2(2.0f, 2.0f));
    AABB box2(Vec2(1.0f, 1.0f), Vec2(3.0f, 3.0f));
    AABB box3(Vec2(5.0f, 5.0f), Vec2(7.0f, 7.0f));

    assert(box1.intersects(box2));
    assert(!box1.intersects(box3));
    assert(box1.contains(Vec2(1.0f, 1.0f)));
    assert(!box1.contains(Vec2(2.5f, 1.0f)));

    std::cout << "    PASSED\n";
}

void test_circle_circle_collision() {
    std::cout << "[+] Running test_circle_circle_collision...\n";
    CircleShape c1(1.0f);
    CircleShape c2(1.0f);
    Manifold m;

    // Overlapping: center distance = 1.5, radius sum = 2.0 -> penetration = 0.5
    bool hit = CollisionDetector::test_circle_circle(c1, Vec2(0.0f, 0.0f), c2, Vec2(1.5f, 0.0f), m);
    assert(hit);
    assert(m.is_colliding);
    assert(std::abs(m.penetration_depth - 0.5f) < 1e-4f);
    assert(std::abs(m.normal.x - 1.0f) < 1e-4f && std::abs(m.normal.y) < 1e-4f);

    // Separated: center distance = 3.0, radius sum = 2.0
    hit = CollisionDetector::test_circle_circle(c1, Vec2(0.0f, 0.0f), c2, Vec2(3.0f, 0.0f), m);
    assert(!hit);
    assert(!m.is_colliding);

    std::cout << "    PASSED\n";
}

void test_polygon_polygon_sat() {
    std::cout << "[+] Running test_polygon_polygon_sat...\n";
    auto boxA = PolygonShape::make_box(1.0f, 1.0f); // 2x2 box centered at (0, 0)
    auto boxB = PolygonShape::make_box(1.0f, 1.0f); // 2x2 box centered at (1.5, 0)
    Manifold m;

    // Overlapping boxes (x overlap from 0.5 to 1.0 -> 0.5 depth)
    bool hit = CollisionDetector::test_polygon_polygon(boxA, Vec2(0.0f, 0.0f), 0.0f,
                                                      boxB, Vec2(1.5f, 0.0f), 0.0f, m);
    assert(hit);
    assert(m.is_colliding);
    assert(std::abs(m.penetration_depth - 0.5f) < 1e-4f);
    assert(m.normal.x > 0.9f); // Points from A towards B

    // Separated boxes
    hit = CollisionDetector::test_polygon_polygon(boxA, Vec2(0.0f, 0.0f), 0.0f,
                                                  boxB, Vec2(3.0f, 0.0f), 0.0f, m);
    assert(!hit);
    assert(!m.is_colliding);

    // Rotated 45-degree box collision check
    float rot45 = 3.14159265f / 4.0f;
    hit = CollisionDetector::test_polygon_polygon(boxA, Vec2(0.0f, 0.0f), rot45,
                                                  boxB, Vec2(1.8f, 0.0f), 0.0f, m);
    assert(hit);
    assert(m.is_colliding);

    std::cout << "    PASSED\n";
}

void test_circle_polygon_sat() {
    std::cout << "[+] Running test_circle_polygon_sat...\n";
    CircleShape circle(1.0f);
    auto box = PolygonShape::make_box(1.0f, 1.0f);
    Manifold m;

    // Circle at (-1.5, 0) and Box at (0, 0) -> Overlap = 1.0 + 1.0 - 1.5 = 0.5
    bool hit = CollisionDetector::test_circle_polygon(circle, Vec2(-1.5f, 0.0f),
                                                      box, Vec2(0.0f, 0.0f), 0.0f, m);
    assert(hit);
    assert(m.is_colliding);
    assert(std::abs(m.penetration_depth - 0.5f) < 1e-4f);
    assert(m.normal.x > 0.9f); // Points from Circle towards Box

    // Separated
    hit = CollisionDetector::test_circle_polygon(circle, Vec2(-3.0f, 0.0f),
                                                 box, Vec2(0.0f, 0.0f), 0.0f, m);
    assert(!hit);

    std::cout << "    PASSED\n";
}

void test_momentum_conservation_elastic_collision() {
    std::cout << "[+] Running test_momentum_conservation_elastic_collision...\n";
    // Zero gravity world to isolate horizontal collision dynamics
    PhysicsWorld world(Vec2(0.0f, 0.0f));

    auto shapeA = std::make_shared<CircleShape>(1.0f);
    auto shapeB = std::make_shared<CircleShape>(1.0f);

    RigidBody* b1 = world.create_body(shapeA, 1.0f, Vec2(-0.8f, 0.0f));
    b1->velocity = Vec2(4.0f, 0.0f);
    b1->restitution = 1.0f; // Perfectly elastic
    b1->friction = 0.0f;

    RigidBody* b2 = world.create_body(shapeB, 1.0f, Vec2(0.8f, 0.0f));
    b2->velocity = Vec2(-4.0f, 0.0f);
    b2->restitution = 1.0f;
    b2->friction = 0.0f;

    float initial_momentum = b1->mass * b1->velocity.x + b2->mass * b2->velocity.x;
    assert(std::abs(initial_momentum) < 1e-5f);

    // Step physics: should resolve collision and invert velocities
    world.step(0.016f);

    float final_momentum = b1->mass * b1->velocity.x + b2->mass * b2->velocity.x;
    assert(std::abs(final_momentum - initial_momentum) < 1e-4f);

    // Elastic bounce: velocities swapped
    assert(b1->velocity.x < 0.0f); // Now moving left
    assert(b2->velocity.x > 0.0f); // Now moving right

    std::cout << "    PASSED\n";
}

void test_static_body_infinite_mass_bounce() {
    std::cout << "[+] Running test_static_body_infinite_mass_bounce...\n";
    PhysicsWorld world(Vec2(0.0f, 0.0f));

    // Static floor at y = 0
    auto floor_shape = std::make_shared<PolygonShape>(PolygonShape::make_box(10.0f, 1.0f));
    RigidBody* floor = world.create_body(floor_shape, 0.0f, Vec2(0.0f, 0.0f)); // mass = 0
    floor->restitution = 1.0f;

    // Dynamic ball colliding downward into floor
    auto ball_shape = std::make_shared<CircleShape>(0.5f);
    RigidBody* ball = world.create_body(ball_shape, 2.0f, Vec2(0.0f, 1.4f)); // penetration = 0.1
    ball->velocity = Vec2(0.0f, -10.0f);
    ball->restitution = 1.0f;

    world.step(0.016f);

    // Static floor must not move
    assert(floor->velocity.x == 0.0f && floor->velocity.y == 0.0f);
    assert(floor->position.x == 0.0f && floor->position.y == 0.0f);

    // Ball should reflect upward with positive velocity
    assert(ball->velocity.y > 0.0f);

    std::cout << "    PASSED\n";
}

void test_rotational_dynamics_and_torque() {
    std::cout << "[+] Running test_rotational_dynamics_and_torque...\n";
    PhysicsWorld world({0.0f, 0.0f}); // Zero gravity

    auto circle = std::make_shared<CircleShape>(1.0f);
    RigidBody* body = world.create_body(circle, 2.0f, Vec2(0.0f, 0.0f));

    // Moment of inertia for solid circle: 0.5 * m * r^2 = 0.5 * 2 * 1 = 1.0
    assert(std::abs(body->inertia - 1.0f) < 1e-5f);
    assert(std::abs(body->inv_inertia - 1.0f) < 1e-5f);

    // Apply constant torque
    body->apply_torque(10.0f);
    world.step(0.1f);

    // omega = torque * inv_inertia * dt = 10 * 1 * 0.1 = 1.0 rad/s
    assert(std::abs(body->angular_velocity - 1.0f) < 1e-4f);
    // rotation = omega * dt = 1.0 * 0.1 = 0.1 rad
    assert(std::abs(body->rotation - 0.1f) < 1e-4f);

    // Test off-center force application
    // Applying force (0, 10) at point (2, 0) gives r = (2, 0), torque = r.cross(f) = 2 * 10 - 0 = 20
    body->apply_force_at_point(Vec2(0.0f, 10.0f), Vec2(2.0f, 0.0f));
    assert(std::abs(body->torque - 20.0f) < 1e-4f);

    world.step(0.1f);
    // Angular velocity increases by 20 * 1 * 0.1 = 2.0 -> total 3.0 rad/s
    assert(std::abs(body->angular_velocity - 3.0f) < 1e-3f);

    std::cout << "    PASSED\n";
}

int main() {
    std::cout << "=== Running physics-2d test suite ===\n";

    test_vec2_operations();
    test_aabb_intersections();
    test_circle_circle_collision();
    test_polygon_polygon_sat();
    test_circle_polygon_sat();
    test_momentum_conservation_elastic_collision();
    test_static_body_infinite_mass_bounce();
    test_rotational_dynamics_and_torque();

    std::cout << "=== All 8 physics-2d tests passed successfully! ===\n";
    return 0;
}
