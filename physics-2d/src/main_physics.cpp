#include "physics2d/body.hpp"
#include "physics2d/shapes.hpp"
#include "physics2d/vec2.hpp"
#include "physics2d/world.hpp"

#include <cmath>
#include <iomanip>
#include <iostream>
#include <memory>
#include <thread>

using namespace gamedev::physics2d;

int main() {
    std::cout << "==========================================================================\n";
    std::cout << "    game-dev-labs // 2D Physics & SAT Collision Resolution Engine         \n";
    std::cout << "       Separating Axis Theorem (SAT) & Impulse-Based Dynamics             \n";
    std::cout << "==========================================================================\n\n";

    // Create World with standard Earth gravity (-9.81 m/s^2)
    PhysicsWorld world(Vec2(0.0f, -9.81f));

    // 1. Static Floor: Box at (0, -0.5), dimensions 20m x 1m (half-extents 10, 0.5)
    auto floor_shape = std::make_shared<PolygonShape>(PolygonShape::make_box(10.0f, 0.5f));
    RigidBody* floor = world.create_body(floor_shape, 0.0f, Vec2(0.0f, -0.5f)); // 0.0 mass = static
    floor->restitution = 0.8f;
    floor->friction = 0.3f;

    // 2. Dynamic Bouncing Sphere: Circle with radius 0.5m dropped from y = 8.0m
    auto ball_shape = std::make_shared<CircleShape>(0.5f);
    RigidBody* ball = world.create_body(ball_shape, 1.5f, Vec2(0.0f, 8.0f));
    ball->restitution = 0.85f; // High bounce
    ball->friction = 0.1f;

    std::cout << "[*] Simulation Setup:\n";
    std::cout << "    Gravity       : " << world.gravity << " m/s^2\n";
    std::cout << "    Floor         : Static AABB at y = -0.5m (mass = INF, restitution = 0.8)\n";
    std::cout << "    Ball          : Circle (radius = 0.5m, mass = 1.5kg, initial y = 8.0m)\n\n";

    std::cout << "--- Telemetry Output (Time, Position, Velocity, Kinetic Energy) ---\n";
    std::cout << std::left << std::setw(8)  << "Step"
              << std::setw(10) << "Time (s)"
              << std::setw(14) << "Pos Y (m)"
              << std::setw(16) << "Vel Y (m/s)"
              << std::setw(14) << "Energy (J)"
              << "ASCII Visual Altitude\n";
    std::cout << "--------------------------------------------------------------------------\n";

    constexpr float dt = 0.02f; // 50 Hz physics tick
    constexpr int TOTAL_STEPS = 90;
    int bounce_count = 0;
    float prev_vy = 0.0f;

    for (int step = 0; step < TOTAL_STEPS; ++step) {
        float time_sec = step * dt;
        float y = ball->position.y;
        float vy = ball->velocity.y;
        float kinetic_energy = 0.5f * ball->mass * (ball->velocity.length_squared());

        // Check for bounce (velocity flips from negative to positive)
        if (prev_vy < -0.5f && vy > 0.0f) {
            ++bounce_count;
        }
        prev_vy = vy;

        // Print telemetry every 3 steps
        if (step % 3 == 0) {
            int bar_len = static_cast<int>(std::max(0.0f, y * 3.0f));
            std::string visual_bar(bar_len, '=');
            visual_bar += " (O)";

            std::cout << std::left << std::setw(8)  << step
                      << std::setw(10) << std::fixed << std::setprecision(2) << time_sec
                      << std::setw(14) << std::setprecision(3) << y
                      << std::setw(16) << std::setprecision(3) << vy
                      << std::setw(14) << std::setprecision(2) << kinetic_energy
                      << visual_bar << "\n";
        }

        world.step(dt);
    }

    std::cout << "\n[+] Simulation finished. Detected " << bounce_count
              << " successful restitution bounces against static ground.\n";
    std::cout << "==========================================================================\n";

    return 0;
}
