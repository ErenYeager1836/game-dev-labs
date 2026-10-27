#include "ecs/components.hpp"
#include "ecs/registry.hpp"
#include "ecs/systems.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>

using namespace gamedev::ecs;

int main(int argc, char* argv[]) {
    size_t entity_count = 50000;
    if (argc > 1) {
        entity_count = static_cast<size_t>(std::stoul(argv[1]));
    }

    std::cout << "==========================================================================\n";
    std::cout << "          game-dev-labs // Entity Component System (ECS) Demo            \n";
    std::cout << "     Data-Oriented Sparse Set Architecture & High-Scale Iteration         \n";
    std::cout << "==========================================================================\n\n";

    Registry registry;
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> pos_dist(-500.0f, 500.0f);
    std::uniform_real_distribution<float> vel_dist(-25.0f, 25.0f);

    std::cout << "[*] Spawning " << entity_count << " game entities with Position & Velocity components...\n";
    auto spawn_start = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < entity_count; ++i) {
        EntityID e = registry.create();
        registry.emplace<Position>(e, pos_dist(rng), pos_dist(rng));
        registry.emplace<Velocity>(e, vel_dist(rng), vel_dist(rng));

        // Assign Health and Renderable to a subset of entities
        if (i % 2 == 0) {
            registry.emplace<Health>(e, 100, 100);
        }
        if (i % 5 == 0) {
            registry.emplace<Renderable>(e, '*', 0x00FF00, 1);
        }
    }

    auto spawn_end = std::chrono::high_resolution_clock::now();
    double spawn_ms = std::chrono::duration<double, std::milli>(spawn_end - spawn_start).count();
    std::cout << "[+] Successfully spawned " << registry.alive_count() << " entities in "
              << std::fixed << std::setprecision(2) << spawn_ms << " ms ("
              << static_cast<int>(entity_count / (spawn_ms / 1000.0)) << " entities/sec)\n\n";

    // Component pool distribution
    std::cout << "--- Component Pool Memory Layout ---\n";
    std::cout << "  Position Components : " << registry.pool<Position>().size() << " contiguous records\n";
    std::cout << "  Velocity Components : " << registry.pool<Velocity>().size() << " contiguous records\n";
    std::cout << "  Health Components   : " << registry.pool<Health>().size() << " contiguous records\n";
    std::cout << "  Renderable Items    : " << registry.pool<Renderable>().size() << " contiguous records\n\n";

    // Run Movement System Benchmark
    constexpr int BENCHMARK_FRAMES = 100;
    const float dt = 1.0f / 60.0f;
    std::cout << "[*] Executing MovementSystem over " << BENCHMARK_FRAMES << " simulation ticks (dt = "
              << std::setprecision(4) << dt << "s)...\n";

    auto sim_start = std::chrono::high_resolution_clock::now();
    size_t total_updates = 0;

    for (int frame = 0; frame < BENCHMARK_FRAMES; ++frame) {
        total_updates += MovementSystem::update(registry, dt);
    }

    auto sim_end = std::chrono::high_resolution_clock::now();
    double sim_ms = std::chrono::duration<double, std::milli>(sim_end - sim_start).count();
    double avg_frame_ms = sim_ms / BENCHMARK_FRAMES;
    double updates_per_sec = (total_updates / (sim_ms / 1000.0));

    std::cout << "[+] Completed " << total_updates << " component updates in "
              << std::setprecision(2) << sim_ms << " ms.\n";
    std::cout << "    Avg Frame Time : " << std::setprecision(3) << avg_frame_ms << " ms ("
              << static_cast<int>(1000.0 / avg_frame_ms) << " simulated FPS equivalent)\n";
    std::cout << "    Throughput     : " << std::setprecision(1) << updates_per_sec / 1e6
              << " Million entity updates/sec\n\n";

    // Demonstrate Combat & ID Recycling
    std::cout << "[*] Simulating combat damage on first 5,000 entities...\n";
    for (EntityID e = 0; e < 5000; ++e) {
        CombatSystem::apply_damage(registry, e, 150); // Lethal damage
    }
    auto dead = CombatSystem::cleanup_dead(registry);
    std::cout << "[+] Dead entities harvested: " << dead.size() << ". Active entities remaining: "
              << registry.alive_count() << "\n";

    // Recycle IDs
    EntityID recycled_1 = registry.create();
    EntityID recycled_2 = registry.create();
    std::cout << "[+] Recreated entities received recycled IDs: " << recycled_1 << ", " << recycled_2 << "\n\n";

    std::cout << "==========================================================================\n";
    std::cout << "ECS demonstration and memory stress test concluded successfully.\n";
    std::cout << "==========================================================================\n";

    return 0;
}
